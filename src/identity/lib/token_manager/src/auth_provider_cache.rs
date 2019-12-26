// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use crate::error::{ResultExt, TokenManagerError};
use crate::AuthProviderSupplier;
use fidl_fuchsia_auth::{AuthProviderProxy, Status};
use fidl_fuchsia_identity_external::OauthOpenIdConnectProxy;
use parking_lot::Mutex;
use std::collections::HashMap;
use std::sync::Arc;

type TokenManagerResult<T> = Result<T, TokenManagerError>;

/// A cache that vends connections to auth providers and uses an `AuthProviderSupplier`
/// to lazily create connections as necessary.
pub struct AuthProviderCache<APS: AuthProviderSupplier> {
    /// The `AuthProviderSupplier` used to generate fresh proxies.
    auth_provider_supplier: APS,
    /// A mapping from auth provider type to existing proxies to `fuchsia.auth.AuthProvider`
    /// implementations.
    auth_provider_proxies: Mutex<HashMap<String, Arc<AuthProviderProxy>>>,
    /// A mapping from auth provider type to existing proxies to
    /// `fuchsia.identity.external.OauthOpenIdConnect` implementations.
    oauth_open_id_connect_proxies: Mutex<HashMap<String, Arc<OauthOpenIdConnectProxy>>>,
}

impl<APS: AuthProviderSupplier> AuthProviderCache<APS> {
    /// Create a new cache that populates itself using the provided `auth_provider_supplier`.
    pub fn new(auth_provider_supplier: APS) -> Self {
        AuthProviderCache {
            auth_provider_supplier,
            auth_provider_proxies: Mutex::new(HashMap::new()),
            oauth_open_id_connect_proxies: Mutex::new(HashMap::new()),
        }
    }

    /// Returns an `AuthProviderProxy` for the specified `auth_provider_type` either by returning
    /// a previously created copy or by acquiring a new one from the `AuthProviderSupplier`.
    pub async fn get_auth_provider_proxy(
        &self,
        auth_provider_type: &str,
    ) -> TokenManagerResult<Arc<AuthProviderProxy>> {
        match self.auth_provider_proxies.lock().get(auth_provider_type) {
            // only return a cached proxy if the connection hasn't crashed.
            Some(auth_provider) if !auth_provider.is_closed() => {
                return Ok(Arc::clone(auth_provider));
            }
            _ => (),
        };

        let client_end = self.auth_provider_supplier.get_auth_provider(auth_provider_type).await?;
        let proxy = Arc::new(client_end.into_proxy().token_manager_status(Status::UnknownError)?);
        self.auth_provider_proxies
            .lock()
            .insert(auth_provider_type.to_string(), Arc::clone(&proxy));
        Ok(proxy)
    }

    /// Returns an `OauthOpenIdConnectProxy` for the specified `auth_provider_type` either by
    /// returning a previously created copy or by acquiring a new one from the
    /// `AuthProviderSupplier`.
    pub async fn get_oauth_open_id_connect_proxy(
        &self,
        auth_provider_type: &str,
    ) -> TokenManagerResult<Arc<OauthOpenIdConnectProxy>> {
        match self.oauth_open_id_connect_proxies.lock().get(auth_provider_type) {
            // only return a cached proxy if the connection hasn't crashed.
            // TODO(satsukiu): possibly record in inspect the number of times that a crash occurs
            // for a given token provider
            Some(oauth_open_id_connect) if !oauth_open_id_connect.is_closed() => {
                return Ok(Arc::clone(oauth_open_id_connect));
            }
            _ => (),
        };

        let client_end =
            self.auth_provider_supplier.get_oauth_open_id_connect(auth_provider_type).await?;
        let proxy = Arc::new(client_end.into_proxy().token_manager_status(Status::UnknownError)?);
        self.oauth_open_id_connect_proxies
            .lock()
            .insert(auth_provider_type.to_string(), Arc::clone(&proxy));
        Ok(proxy)
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::fake_auth_provider_supplier::FakeAuthProviderSupplier;
    use fidl_fuchsia_auth::{AuthProviderRequest, AuthProviderStatus};
    use fidl_fuchsia_identity_external::{
        OauthOpenIdConnectRequest, OpenIdTokenFromOauthRefreshTokenRequest,
    };
    use fidl_fuchsia_identity_tokens::OpenIdToken;
    use fuchsia_async as fasync;
    use futures::future::join;
    use futures::prelude::*;

    #[fasync::run_until_stalled(test)]
    async fn test_get_auth_provider_proxy() {
        let auth_provider_supplier = FakeAuthProviderSupplier::new();

        auth_provider_supplier.add_auth_provider("crashes", |mut stream| {
            async move {
                match stream.try_next().await? {
                    Some(AuthProviderRequest::GetAppAccessToken { responder, .. }) => {
                        responder.send(AuthProviderStatus::Ok, None)?
                    }
                    _ => panic!("Got unexpected request"),
                };

                // close stream on second request to simulate crash
                let _ = stream.try_next().await?;
                std::mem::drop(stream);
                Ok(())
            }
        });

        let auth_provider_fut = auth_provider_supplier.run();

        let test_fut = async move {
            let cache = AuthProviderCache::new(auth_provider_supplier);

            // first get call should create a fresh proxy
            let (status, _) = cache
                .get_auth_provider_proxy("crashes")
                .await?
                .get_app_access_token("credential", None, &mut vec![].into_iter())
                .await
                .unwrap();
            assert_eq!(status, AuthProviderStatus::Ok);

            // second get call should retrieve the same proxy, the auth provider should fail
            assert!(cache
                .get_auth_provider_proxy("crashes")
                .await?
                .get_app_access_token("credential", None, &mut vec![].into_iter())
                .await
                .is_err());

            // third get call should attempt to create a new proxy since the provider crashed.
            // Since FakeAuthProviderSupplier currently can't make the same auth provider twice
            // the get should fail.
            assert_eq!(
                cache.get_auth_provider_proxy("crashes").await.unwrap_err().status,
                Status::AuthProviderServiceUnavailable
            );

            TokenManagerResult::<()>::Ok(())
        };

        let (auth_provider_res, test_res) = join(auth_provider_fut, test_fut).await;
        assert!(test_res.is_ok());
        assert!(auth_provider_res.is_ok());
    }

    #[fasync::run_until_stalled(test)]
    async fn test_get_oauth_open_id_proxy() {
        let auth_provider_supplier = FakeAuthProviderSupplier::new();

        auth_provider_supplier.add_oauth_open_id_connect("crashes", |mut stream| {
            async move {
                match stream.try_next().await? {
                    Some(OauthOpenIdConnectRequest::GetIdTokenFromRefreshToken {
                        responder,
                        ..
                    }) => {
                        responder.send(&mut Ok(OpenIdToken { content: None, expiry_time: None }))?
                    }
                    _ => panic!("Got unexpected request"),
                };

                // close stream on second request to simulate crash
                let _ = stream.try_next().await?;
                std::mem::drop(stream);
                Ok(())
            }
        });

        let auth_provider_fut = auth_provider_supplier.run();

        let test_fut = async move {
            let cache = AuthProviderCache::new(auth_provider_supplier);

            // first get call should create a fresh proxy
            cache
                .get_oauth_open_id_connect_proxy("crashes")
                .await?
                .get_id_token_from_refresh_token(OpenIdTokenFromOauthRefreshTokenRequest {
                    refresh_token: None,
                    audiences: None,
                })
                .await
                .unwrap()
                .unwrap();

            // second get call should retrieve the same proxy, the auth provider should fail
            assert!(cache
                .get_oauth_open_id_connect_proxy("crashes")
                .await?
                .get_id_token_from_refresh_token(OpenIdTokenFromOauthRefreshTokenRequest {
                    refresh_token: None,
                    audiences: None
                })
                .await
                .is_err());

            // third get call should attempt to create a new proxy since the provider crashed.
            // Since FakeAuthProviderSupplier currently can't make the same auth provider twice
            // the get should fail.
            assert_eq!(
                cache.get_oauth_open_id_connect_proxy("crashes").await.unwrap_err().status,
                Status::AuthProviderServiceUnavailable
            );

            TokenManagerResult::<()>::Ok(())
        };

        let (auth_provider_res, test_res) = join(auth_provider_fut, test_fut).await;
        assert!(test_res.is_ok());
        assert!(auth_provider_res.is_ok());
    }
}
