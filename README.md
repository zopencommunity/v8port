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



## z/OS Open Tools Workflow 

This is the workflow for building v8 using the z/OS Open Tools framework.

1. install depot_tools 
    - `$ zopen install depot_tools`
2. Currently v8port is still in developer form
    - `$ cd ${HOME}/zopen/dev`
    - `$ git clone git@github.com:ZOSOpenTools/v8port.git`
3. The build process will use git via ssh.  If you have a passphrase
   for your ssh key, do the following:
   - `$ eval \`ssh-agent\``  
   - `$ ssh-add ${HOME}/.ssh/some_private_ssh_key`
4. Start the build process
    - `$ cd ${HOME}/zopen/dev/v8port`
    - `$ rm -rf ~/.local v8base install ${HOME}/zopen/usr/local/zopen/v8 venv log; zopen build > out.log 2>&1`

It will create a venv in `${HOME}/zopen/usr/local/zopen/v8/v8-DEV/venv`.
The v8port `buildenv` file will setup config the environment to 
fetch the v8 source code. Afterwards, source this venv via
`. ${HOME}/zopen/usr/local/zopen/v8/v8-DEV/venv/bin/activate`
to work on the repo.

This can be done in a convienece script via

```
$ . ./setenv.sh
```

or if you want to preserve the hidden `v8base/.gclient` and `v8base/.gclient_entries`

```
$ . ./setenv.sh -resume
```



