#!/boot/bin/sh

export FIDL_COMPATIBILITY_TEST_SERVERS=fidl_compatibility_test_server_cpp,fidl_compatibility_test_server_go,fidl_compatibility_test_server_rust
/pkgfs/packages/fidl_compatibility_test_bin/0/bin/app
