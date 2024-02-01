[![Automatic version updates](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml/badge.svg)](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml)

# v8

A port of Google V8 JavaScript engine to z/OS Open Tools project

## Workflow 

### Applying the patches
Since the [cipd executable](https://chromium.googlesource.com/infra/luci/luci-go/+/master/cipd/) is not yet ported on z/OS, the following steps are
currently used to update V8 and its dependencies, and apply the z/OS port, after
the initial [fetch v8](https://v8.dev/docs/source-code) (also included below with a comment).

This workflow requires two platforms.  One is z/OS® UNIX System Services (z/OS UNIX) which is the target platform.  The second is a generic linux
AMD64 platform to get files normally acquired via `cipd`


#### Linux plaform (non z/OS® UNIX System Services (z/OS UNIX)) setup

This is the workflow on the non z/OS® UNIX System Services (z/OS UNIX) platform for obtaining files via `cipd`.  Do
this once.

```
$ cd $HOME
$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
$ export PATH=~/depot_tools:$PATH
$ fetch v8
$ cd v8
$ git checkout main
$ git reset fce38915e4c7c73c0e7f77bb13e2a78514968d35 --hard
$ tar -cvf x.tar test tools third_party
```

Note, the resultant tar file is 2.4GB.  Use sftp rather than scp
to transfer the file to z/OS® UNIX System Services (z/OS UNIX). 
Due to large file size use md5sum to verify the hash on 
each platform like so:

```
$ md5sum x.tar
```


#### z/OS® UNIX System Services (z/OS UNIX) platform setup

This is the workflow on the z/OS® UNIX System Services (z/OS UNIX) platform.

##### Do this once

This uses zopen depot tools port to obtain a copy of the v8 source
according to the v8 workflow.

```
$ mkdir $HOME/zopen/dev/v8base
$ cd $HOME/zopen/dev/v8base
$ fetch v8
```

Obtain a valid git repo.

```
$ cd $HOME/zopen/dev/v8base/v8
$ git checkout main
```

##### Do this afterwards and everytime you need to sync

Once you have done the intial setup above, these are the
steps to sync the code to the specified revision and apply the corresponding
files from the Linux AMD64 platform for the files normally 
acquired via `cipd`.

The reset command is required as the current patches are valid as of the specified commit.

```
$ cd $HOME/zopen/dev/v8base/v8
$ git pull
$ git reset fce38915e4c7c73c0e7f77bb13e2a78514968d35 --hard
$ gclient sync
$ rm -rf test tools third_party
```

At this point the file system is ready to be suplemented with the files
in the tar file from the Linux AMD64 platform.

```
$ cd $HOME/zopen/dev/v8base/v8
$ tar -xvf ~/zopen/x.tar
```

Output:

```
JD895801@USILCA31 v8 (main)
$ tar -xvf ~/zopen/x.tar
tar: This does not look like a tar archive
tar: Skipping to next header
\244\201\223\242M\363k@\201\204\204M\207k@\206]]^\025
tar: Skipping to next header
\345\301\323\344\305@L@N\311\225\206\211\225\211\243\250]@~~~@\243\231\244\205}]^\025\320\025
tar: Skipping to next header
a\025\025\211\206@M\243\231\244\205]@\300\025@@\211\224\227\226\231\243M}}]^\025\320\025
tar: Skipping to next header
tar: Exiting with failure status due to previous errors
JD895801@USILCA31 v8 (main)
$ type tar
tar is hashed (/z/jd895801/zopen/usr/local/bin/tar)
```


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
v8port/patches/. Adjust the path to v8port to match your environment. Example:
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
