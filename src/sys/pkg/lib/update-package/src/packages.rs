// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    fidl_fuchsia_io::DirectoryProxy,
    fuchsia_url::pkg_url::PkgUrl,
    fuchsia_zircon::Status,
    serde::{Deserialize, Serialize},
    thiserror::Error,
};

#[derive(Serialize, Deserialize, Debug)]
#[serde(tag = "version", content = "content", deny_unknown_fields)]
enum Packages {
    #[serde(rename = "1")]
    V1(Vec<PkgUrl>),
}

impl Packages {
    #[cfg(test)]
    fn unwrap(self) -> Vec<PkgUrl> {
        match self {
            Packages::V1(pkgs) => pkgs,
        }
    }
}

/// ParsePackageError represents any error which might occur while reading either
/// `packages` or `packages.json` from an update package.
#[derive(Debug, Error)]
#[allow(missing_docs)]
pub enum ParsePackageError {
    #[error("'{0}' is an invalid line for a `packages` file")]
    InvalidLine(String),

    #[error("could not open '{0}'")]
    FailedToOpen(&'static str, io_util::node::OpenError),

    #[error("could not parse url")]
    URLParseError(fuchsia_url::errors::ParseError),

    #[error("error reading file '{0}'")]
    ReadError(&'static str, io_util::file::ReadError),

    #[error("json parsing error while reading `packages.json`")]
    JsonError(#[source] serde_json::error::Error),
}

/// Takes a string which is a newline seperated list of <package name>/<variant>=<hash> lines
/// and returns a list of parsed PkgUrls.
pub(crate) fn parse_packages(contents: &str) -> Result<Vec<PkgUrl>, ParsePackageError> {
    let mut out: Vec<PkgUrl> = vec![];
    for line in contents.split("\n") {
        if line == "" {
            continue;
        }
        let mut parts = line.trim().split('=');
        let err = || ParsePackageError::InvalidLine(line.to_owned());
        let key = parts.next().ok_or_else(err)?;
        let value = parts.next().ok_or_else(err)?.to_string();
        if let Some(_) = parts.next() {
            return Err(err());
        }
        let url = PkgUrl::new_package("fuchsia.com".to_string(), format!("/{}", key), Some(value));
        out.push(url.map_err(ParsePackageError::URLParseError)?);
    }
    Ok(out)
}

pub(crate) fn parse_packages_json(contents: &str) -> Result<Vec<PkgUrl>, ParsePackageError> {
    match serde_json::from_str(&contents).map_err(ParsePackageError::JsonError)? {
        Packages::V1(pkgs) => Ok(pkgs),
    }
}

async fn legacy_packages(proxy: &DirectoryProxy) -> Result<Vec<PkgUrl>, ParsePackageError> {
    let file =
        io_util::directory::open_file(proxy, "packages", fidl_fuchsia_io::OPEN_RIGHT_READABLE)
            .await
            .map_err(|e| ParsePackageError::FailedToOpen("packages", e))?;

    let contents = io_util::file::read_to_string(&file)
        .await
        .map_err(|e| ParsePackageError::ReadError("packages", e))?;
    parse_packages(&contents)
}

/// Returns the list of package urls that go in the universe of this update package.
pub(crate) async fn packages(proxy: &DirectoryProxy) -> Result<Vec<PkgUrl>, ParsePackageError> {
    let file = match io_util::directory::open_file(
        &proxy,
        "packages.json",
        fidl_fuchsia_io::OPEN_RIGHT_READABLE,
    )
    .await
    {
        Ok(file) => Ok(file),
        Err(io_util::node::OpenError::OpenError(Status::NOT_FOUND)) => {
            return legacy_packages(proxy).await
        }
        Err(e) => Err(ParsePackageError::FailedToOpen("packages.json", e)),
    }?;

    let contents = io_util::file::read_to_string(&file)
        .await
        .map_err(|e| ParsePackageError::ReadError("packages.json", e))?;
    parse_packages_json(&contents)
}

#[cfg(test)]
mod tests {
    use {super::*, crate::TestUpdatePackage, matches::assert_matches, serde_json::json};

    fn pkg_urls(v: Vec<&str>) -> Vec<PkgUrl> {
        v.into_iter().map(|s| PkgUrl::parse(s).unwrap()).collect()
    }

    #[test]
    fn smoke_test_parse_packages_lines() {
        let packages = "ls/0=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea\npkg-resolver/0=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0";
        let expected_packages = pkg_urls(vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ]);
        assert_eq!(parse_packages(packages).unwrap(), expected_packages);
    }

    #[test]
    fn smoke_test_parse_packages_lines_empty() {
        let packages =
            "\n\n\nls/0=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea\n\n\n\n\n\n\npkg-resolver/0=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0\n\n\n\n";
        let expected_packages = pkg_urls(vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ]);
        assert_eq!(parse_packages(packages).unwrap(), expected_packages);
    }

    #[test]
    fn expect_error_on_extra_space() {
        let packages = "    ls/0 =     71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea  \npkg-resolver/0   =26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0 ";
        assert_matches!(parse_packages(packages), Err(ParsePackageError::URLParseError(_)));
    }

    #[test]
    fn parse_packages_empty() {
        let packages = parse_packages("").unwrap();
        assert_eq!(packages, vec![]);
    }

    #[test]
    fn expect_failure_parse_packages() {
        assert_matches!(parse_packages("{}"), Err(ParsePackageError::InvalidLine(_)));
    }

    #[test]
    fn smoke_test_parse_packages_json() {
        let packages = Packages::V1(pkg_urls(vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ]));
        let packages_json = serde_json::to_string(&packages).unwrap();
        assert_eq!(parse_packages_json(&packages_json).unwrap(), packages.unwrap());
    }

    #[test]
    fn expect_failure_parse_packages_json() {
        assert_matches!(parse_packages_json("{}"), Err(ParsePackageError::JsonError(_)));
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn smoke_test_packages_lines() {
        let update_pkg = TestUpdatePackage::new();
        let packages = "ls/0=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea\npkg-resolver/0=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0";
        let expected_packages = pkg_urls(vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ]);
        let update_pkg = update_pkg.add_file("packages", packages).await;
        assert_eq!(update_pkg.packages().await.unwrap(), expected_packages);
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn smoke_test_packages_json() {
        let update_pkg = TestUpdatePackage::new();
        let pkg_list = vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ];
        let packages = json!({
            "version": "1",
            "content": pkg_list,
        })
        .to_string();
        let update_pkg = update_pkg.add_file("packages.json", packages).await;
        assert_eq!(update_pkg.packages().await.unwrap(), pkg_urls(pkg_list));
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn expect_failure_json() {
        let update_pkg = TestUpdatePackage::new();
        let packages = "{}";
        let update_pkg = update_pkg.add_file("packages.json", packages).await;
        assert_matches!(update_pkg.packages().await, Err(ParsePackageError::JsonError(_)))
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn expect_failure_no_files() {
        let update_pkg = TestUpdatePackage::new();
        assert_matches!(
            update_pkg.packages().await,
            Err(ParsePackageError::FailedToOpen("packages", _))
        )
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn expect_failure_lines() {
        let update_pkg = TestUpdatePackage::new();
        let packages = "{}";
        let update_pkg = update_pkg.add_file("packages", packages).await;
        assert_matches!(update_pkg.packages().await, Err(ParsePackageError::InvalidLine(_)))
    }

    #[fuchsia_async::run_singlethreaded(test)]
    async fn expect_json_not_lines() {
        let update_pkg = TestUpdatePackage::new();
        let pkg_list = vec![
            "fuchsia-pkg://fuchsia.com/ls/0?hash=71bad1a35b87a073f72f582065f6b6efec7b6a4a129868f37f6131f02107f1ea",
            "fuchsia-pkg://fuchsia.com/pkg-resolver/0?hash=26d43a3fc32eaa65e6981791874b6ab80fae31fbfca1ce8c31ab64275fd4e8c0",
        ];
        let packages = json!({
            "version": "1",
            "content": pkg_list,
        })
        .to_string();
        let update_pkg = update_pkg
            .add_file("packages.json", packages)
            .await
            .add_file("packages", "ls/0=00b001a00b00a000f00f50006006b00fe00b6a00129008f300613000210001ea\npkg-resolver/0=00003a00c300aa00e6001001800b6008000e300bf001c00c300b60075004e800")
            .await;
        assert_eq!(update_pkg.packages().await.unwrap(), pkg_urls(pkg_list));
    }
}
