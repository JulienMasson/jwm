#tcase Utils

#test file_access_test
    fail_unless(file_access("/etc/passwd") == true, "Failed to access /etc/passwd");
    fail_unless(file_access("/etc/tatatat") == false, "Shouldn't access to /etc/tatatat");
