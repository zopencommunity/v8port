[![Automatic version updates](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml/badge.svg)](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml)

# v8 port

A port of Google V8 JavaScript engine to z/OS Open Tools project

The upstream for v8 is [here.](https://v8.dev)

## Developer Notes

### Prerequisite

The manual workflow to build v8port requires the following:

* python
    - tested with python 3.11 from base z/OS install
* ninja from z/OS Open Tools [ninjaport](https://github.com/ZOSOpenTools/ninjaport)
* gn from zos open tools [gnport](https://github.com/ZOSOpenTools/gnport)
    - clang++ from [IBM C/C++ for Open Enterprise Languages on z/OS 2.0](https://epwt-www.mybluemix.net/software/support/trial/cst/programwebsite.wss?siteId=1803)
    ```
    $ clang++ --version
    C/C++ for Open Enterprise Languages on z/OS 2.0.0, clang version 14.0.0 (build 37a9321)
    Target: s390x-ibm-zos
    Thread model: posix
    InstalledDir: /C/CCplus/LangV2GA/usr/lpp/IBM/oelcpp/v2r0/bin
    ```
* depot_tools port  zos open tools depot_toolsport
* zoslib
    - The manual workflow provides instructions on how to install this.  However, 
    its noted here to catalog the compiler.  With that said, it uses the same `clang++` above and
    `clang` from the same package.
    ```
    $ clang --version
    C/C++ for Open Enterprise Languages on z/OS 2.0.0, clang version 14.0.0 (build 37a9321)
    Target: s390x-ibm-zos
    Thread model: posix
    InstalledDir: /C/CCplus/LangV2GA/usr/lpp/IBM/oelcpp/v2r0/bin
    ```



## Manual Workflow 

#### Applying the patches
Since the [cipd executable](https://chromium.googlesource.com/infra/luci/luci-go/+/master/cipd/) is not yet ported on z/OS, the following steps are
currently used to update V8 and its dependencies, and apply the z/OS port, after
the initial [fetch v8](https://v8.dev/docs/source-code) (also included below with a comment).

This workflow requires two platforms.  One a z/OS® UNIX System Services (z/OS UNIX) which is 
the target platform.  The second is a generic platform used to get files normally acquired via `cipd`.


### Linux platform (non z/OS® UNIX System Services (z/OS UNIX)) setup

This is the workflow on the non-z/OS UNIX platform for obtaining files via `cipd`.  
There are two aspects to this workflow.  One which is done once and one which
is done everytime you want to sync.

This workflow is done once.

```
$ cd $HOME
$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
$ export PATH=~/depot_tools:$PATH
```

As an aside, if you have done this already and you are trying to repeat
from a pristine state, remove all the old `.gclient*` files at the 
level of v8 before continuing.

```
$ cd $HOME
$ rm .gclient*
```

```
$ fetch v8
$ cd $HOME/v8
$ git checkout main
```
This workflow is done afterwards and everytime you want to sync.

```
$ cd $HOME/v8
$ git pull
$ git reset fce38915e4c7c73c0e7f77bb13e2a78514968d35 --hard
$ gclient sync -D
$ tar -cvf x.tar test tools third_party
```

Note, the resultant tar file is 2.4GB.  Use sftp rather than scp
to transfer the file to z/OS UNIX platform.
Due to large file size use md5sum to verify the hash on 
each platform like so:

```
$ md5sum x.tar
```


### z/OS® UNIX System Services (z/OS UNIX) platform setup

This is the workflow on the z/OS® UNIX System Services (z/OS UNIX) platform.

It assumes you are using z/OS Open Tools framework and the associated install
of depot_toolsport.  Follow the workflow described [here](https://github.com/ZOSOpenTools/depot_toolsport?tab=readme-ov-file#developer-notes) to ensure you have depot tools properly installed 
and configured.

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

Once you have done the initial setup above, these are the
steps to sync the code to the specified revision and apply the corresponding
files from the Linux AMD64 platform for the files normally 
acquired via `cipd`.

The reset command is required as the current patches are valid as of the specified commit.

```
$ cd $HOME/zopen/dev/v8base/v8
$ git pull
$ git reset fce38915e4c7c73c0e7f77bb13e2a78514968d35 --hard
$ gclient sync -D
$ rm -rf test tools third_party
```

At this point the file system is ready to be suplemented with the files
in the tar file from the non z/OS UNIX platform.

```
$ cd $HOME/zopen/dev/v8base/v8
$ tar -xvf $HOME/zopen/x.tar
```

Then make sure all the files are properly tagged as ASCII:

```
$ cd $HOME/zopen/dev/v8base/v8
$ chtag -tc819 -R test tools third_party
```

Optionally remove the tar file as its not longer needed.  It will 
need to be replaced if sync is performed, however keep in mind
this will also require the workflow on the non z/OS UNIX platform 
to recreate it.

```
$ rm x.tar
```

Ensure that the commit hash on each platform at the top matches.

On each system:

```
$ git log --oneline
```

##### Apply patches and pax files on z/OS UNIX platform

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

##### Patch Workflow

This is a large patch which updates multiple portions of the codebase.
```
$ cd $HOME/zopen/dev/v8base/v8
$ git apply ../../v8port/patches/git.20231122.fce38915e4.diff
```

This is a small patch which updates the toolchain config.
```
$ cd $HOME/zopen/dev/v8base/v8
$ pushd build
$ git apply ../../../v8port/patches/build/git.20231122.968682938b.diff
$ popd
```
From this point forward, `pushd/popd` is used so that the directory
is not specifed each time.

This is a small patch which updates the third-party abseil common c++ libaries.
```
$ pushd third_party/abseil-cpp
$ git apply ../../../../v8port/patches/third_party/abseil-cpp/git.20231122.1e8861f03f.diff
$ popd
```

This is a tiny patch which adds a new ZOS platform to google test.
```
$ pushd third_party/googletest/src
$ git apply ../../../../../v8port/patches/third_party/googletest/src/git.20231122.af29db7ec2.diff
$ popd
```


##### PAX Workflow

Extract the files from the .pax files, by running `pax -p p -rzf <pax-file>`,
using the same approach. Example:

```
$ cd $HOME/zopen/dev/v8base/v8
$ pax -p p -rzf ../../v8port/patches/src.zos.20231122.fce38915e4.pax

$ pushd buildtools
$ pax -p p -rzf ../../../v8port/patches/buildtools/buildtools.zos.20231122.92b79f4d75.pax
$ popd
```


##### Download, build and install zoslib:


```
$ cd $HOME/zopen/dev/v8base/v8/third_party
$ git clone git@github.com:ibmruntimes/zoslib 
$ cd zoslib
```

Ensure the toolchain is set properly.  These settings
are also made in the depot_toolsport/setenv.sh script.

```
$ export CC=clang
$ export CXX=clang++
$ export LINK=clang++
```

Build zoslib.

```
$ cd $HOME/zopen/dev/v8base/v8/third_party/zoslib
$ ./build.sh -c -r -t
```


Even though zoslib is built as part of V8 as a static library, the shared
library version currently has to be built manually, and is used by
`IsS390SimdSupported()` in `tools/testrunner/local/utils.py`, where it's
dynamically loaded, with its directory extracted from the `ZOSLIB_LIBPATH`
environment variable. So set the variable before running the tests:



```
$ cd $HOME/zopen/dev/v8base/v8
$ export ZOSLIB_LIBPATH=`pwd`/third_party/zoslib/install/lib
```


##### Build V8


Prepare v8 for building.

```
$ cd $HOME/zopen/dev/v8base/v8
$ export PATH=`pwd`/buildtools/zos:$PATH # for the 'ar' wrapper
$ pushd buildtools/zos
$ ln -s $HOME/zopen/dev/gnport/gn/out/gn .
$ popd
```

Do the build.

```
$ gn -v gen out/zos_s390x.release --args="is_debug=false treat_warnings_as_errors=false" 
$ V=1 ninja -v -j 12 -C out/zos_s390x.release/
```




## z/OS Open Tools Workflow 

This is the workflow for building v8 using the z/OS Open Tools framework.

1. install depot_tools 
    - `$ zopen install depot_tools`
2. Currently v8port is still in developer form
    - `$ cd ${HOME}/zopen/dev`
    - `$ git clone git@github.com:ZOSOpenTools/v8port.git`
3. Start the build process
    - `$ cd ${HOME}/zopen/dev/v8port`
    - `$ rm -rf ~/.local v8base install ${HOME}/zopen/usr/local/zopen/v8; zopen build`

It will create a venv in `${HOME}/zopen/usr/local/zopen/v8/v8-DEV/venv`.
The v8port `buildenv` file will setup config the environment to 
fetch the v8 source code. 



