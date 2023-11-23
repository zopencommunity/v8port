v8

A port of Google V8 JavaScript engine to z/OS Open Tools project

# Applying the patches
Since the [cipd executable](https://chromium.googlesource.com/infra/luci/luci-go/+/master/cipd/) is not yet ported on z/OS, the following steps are
currently used to update V8 and its dependencies, and apply the z/OS port, after
the initial [fetch v8](https://v8.dev/docs/source-code).

```
cd v8
git pull
pushd ..
gclient sync
popd
rm -rf test tools third_party
```
From another platform (e.g. Linux), perform the same steps above (but without
the rm command), then transfer the directories test, tools and third_party to
your z/OS machine.

Create an archive using `tar` (on the non-z/OS platform):
```
cd v8
tar -cvf x.tar test tools third_party
```
Transfer x.tar to the z/OS machine using your preferred method (e.g. `sftp`).

Then on the z/OS machine:
```
cd v8
# extract the archive:
tar -xvf x.tar
# tag all files in those directories as ASCII:
chtag -tc819 -R test tools third_party
rm x.tar
```

Ensure that the head commit on both z/OS and the other platform are the same; if
not then `git reset <commit> --hard` on z/OS to match the commit on the other
platform.

The .pax files contain only z/OS files, and are intended to easily extract its
content to include those files' directories. Those files are also available in
the repository for review.

Patch files are named without `.patch` so they're not processed by zopen. The
date in the filenames is the date on which the V8 repository and its
dependencies were pulled; the commit is the head commit of the corresponding
repository at the time of the pull.

From your local V8 repository or dependency, run `git apply` on the
`git.<date>.<commit>.diff` file that's under the corresponding directory in
v8port/patches/. Example:
```
cd v8
git apply ~/v8port/patches/git.20231122.fce38915e4.diff

pushd build
git apply ~/v8port/patches/build/git.20231122.968682938b.diff
popd

pushd third_party/abseil-cpp
git apply ~/v8port/patches/third_party/abseil-cpp/git.20231122.1e8861f03f.diff
popd

pushd third_party/googletest/src
git apply ~/v8port/patches/third_party/googletest/src/git.20231122.af29db7ec2.diff
popd
```
# Files specific to z/OS only
Extract the files from the .pax files, by running `pax -p p -rzf <pax-file>`,
using the same approach. Example:

```
cd v8
pax -p p -rzf ~/v8port/patches/src.zos.20231122.fce38915e4.pax

pushd buildtools
pax -p p -rzf ~/v8port/patches/buildtools/buildtools.zos.20231122.92b79f4d75.pax
popd
```
# Download, build and install zoslib:
```
pushd v8/third_party
git clone git@github.com:ibmruntimes/zoslib && cd zoslib
./build.sh -c -r -t
popd
```
Even though zoslib is built as part of V8 as a static library, the shared
library version currently has to be built manually, and is used by
`IsS390SimdSupported()` in `tools/testrunner/local/utils.py`, where it's
dynamically loaded, with its directory extracted from the `ZOSLIB_LIBPATH`
environment variable. So set the variable before running the tests:
```
cd v8
export ZOSLIB_LIBPATH=`pwd`/third_party/zoslib/install/lib
```
# Build V8
```
cd v8
export PATH=`pwd`/buildtools/zos:$PATH # for the 'ar' wrapper
gn -v gen out/zos_s390x.release --args="is_debug=false treat_warnings_as_errors=false"
V=1 ninja -v -j 12 -C out/zos_s390x.release/
```
