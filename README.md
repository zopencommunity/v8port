[![Automatic version updates](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml/badge.svg)](https://github.com/ZOSOpenTools/v8port/actions/workflows/bump.yml)

# v8

A port of Google V8 JavaScript engine to z/OS Open Tools project

## Workflow 

### Applying the patches
Since the [cipd executable](https://chromium.googlesource.com/infra/luci/luci-go/+/master/cipd/) is not yet ported on z/OS, the following steps are
currently used to update V8 and its dependencies, and apply the z/OS port, after
the initial [fetch v8](https://v8.dev/docs/source-code) (also included below with a comment).

This workflow requires two platforms.  One a z/OS® UNIX System Services (z/OS UNIX) which is 
the target platform.  The second is a generic platform used to get files normally acquired via `cipd`.


#### Linux plaform (non z/OS® UNIX System Services (z/OS UNIX)) setup

This is the workflow on the non-z/OS UNIX platform for obtaining files via `cipd`.  
There are two aspects to this workflow.  One which is done once and one which
is done everytime you want to sync.

This workflow is done once.

```
$ cd $HOME
$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
$ export PATH=~/depot_tools:$PATH
$ fetch v8
$ cd $HOME/v8
$ git checkout main
```
This workflow is done afterwards and everytime you want to sync.

```
$ cd $HOME/v8
$ git pull
$ git reset fce38915e4c7c73c0e7f77bb13e2a78514968d35 --hard
$ gclient sync
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

It assumes you are using z/OS Open Tools framework and the associated install
of depot_toolsport.  Follow the workflow described [here](https://github.com/ZOSOpenTools/depot_toolsport?tab=readme-ov-file#developer-notes) to ensure you have deeopot tools properly installed 
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

Once you have done the intial setup above, these are the
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
$ tar -xvf ~/zopen/x.tar
```

Then make sure all the files are properly tagged as ASCII:

```
$ cd $HOME/zopen/dev/v8base/v8
$ chtag -tc819 -R test tools third_party
```

Optionally remove the tar file as its not longer needed.  It will 
need to be replaced if sync is performed, however keep in mind
this will also require the workflow on the non z/OS UNIX system 
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

Ensure the toolchain is set properly.

```
$ export CC=clang
$ export CXX=clang++
$ export LINK=$CXX
```

Build zoslib.

```
$ cd $HOME/zopen/dev/v8base/v8/third_party/v8
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


```
$ gn -v gen out/zos_s390x.release --args="is_debug=false treat_warnings_as_errors=false" 
$ V=1 ninja -v -j 12 -C out/zos_s390x.release/
```

The gn command looks like it worked.

output:

```
... stuff ommitted
Computing //test/unittests:generate-bytecode-expectations(//build/toolchain/zos:s390x)
Computing //test/unittests:inspector_unittests_sources(//build/toolchain/zos:s390x)
Computing //test:gn_all(//build/toolchain/zos:s390x)
Computing //:gn_all(//build/toolchain/zos:s390x)
Computing //test/unittests:v8_heap_base_unittests_sources(//build/toolchain/zos:s390x)
Build graph constructed in 2810ms
Done. Made 494 targets from 126 files in 2824ms
```

The next command failed though.

output:

```
$ V=1 ninja -v -j 12 -C out/zos_s390x.release/
Traceback (most recent call last):
  File "/z/jd895801/zopen/dev/depot_toolsport/depot_tools/ninja.py", line 94, in <module>
    sys.exit(main(sys.argv))
             ^^^^^^^^^^^^^^
  File "/z/jd895801/zopen/dev/depot_toolsport/depot_tools/ninja.py", line 87, in main
    return subprocess.call([ninja_path] + args[1:])
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lpp/IBM/cyp/v3r11/pyz/lib/python3.11/subprocess.py", line 389, in call
    with Popen(*popenargs, **kwargs) as p:
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lpp/IBM/cyp/v3r11/pyz/lib/python3.11/subprocess.py", line 1026, in __init__
    self._execute_child(args, executable, preexec_fn, close_fds,
  File "/usr/lpp/IBM/cyp/v3r11/pyz/lib/python3.11/subprocess.py", line 1950, in _execute_child
    raise child_exception_type(errno_num, err_msg, err_filename)
OSError: [Errno 130] EDC5130I Exec format error.: '/z/jd895801/zopen/dev/v8base/v8/third_party/ninja/ninja'
(myenv) JD895801@USILCA31 v8 (main)
```

This is where I am picking up ninja

```
$ type ninja
ninja is hashed (/z/jd895801/zopen/dev/depot_toolsport/depot_tools/ninja)
```



