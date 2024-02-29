# Apply before running `gclient sync` while under the `v8` directory:
# sed -i -f <path-to>/v8port/patches/DEPS.sed DEPS
#
# Disable download of zos-s390x packages not present upstream; those that
# are required (clang, ninja, gn) are already available on z/OS; others
# such as reclient and luci-go, are currently not required on z/OS:
#
s?host_cpu != "s390"?host_cpu != "s390" and host_os != "zos"?
s?'condition': 'host_os != "aix"'?'condition': 'host_os != "aix" and host_os != "zos"'?
#
# Also force download of the depot_tools commit that includes the z/OS
# port, so `gclient sync` works on the current v8port patches, until the
# those patches are updated for a more recent V8 head commit than the
# current (fce38915 from Nov 22 2023):
#
s?'38b8de056ebb9d0aa138f09907fa83c5b6107bb5'?'9d64acedead8bf69b1eee2645a18f8fd47a1100d'?
