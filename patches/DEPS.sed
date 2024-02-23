# Disable download of zos-s390x packages not present upstream; those that
# are required (clang, ninja, gn) are already available on z/OS; others
# such as reclient and luci-go, are currently not required on z/OS.
#
# Apply before running `gclient sync` while under the `v8` directory:
# sed -i -f <path-to>/v8port/patches/DEPS.sed DEPS
#
s?host_cpu != "s390"?host_cpu != "s390" and host_os != "zos"?
s?'condition': 'host_os != "aix"'?'condition': 'host_os != "aix" and host_os != "zos"'?
