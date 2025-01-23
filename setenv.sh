#!/usr/bin/env bash


export PATH="${HOME}/zopen/usr/local/bin:${DEPOT_TOOLS_HOME}:${GN_HOME}:$PATH"

# update the compiler settings
unset CFLAGS CXXFLAGS LDFLAGS

export DEPOT_TOOLS_UPDATE=0
# do the export and the set
export CUSTOM_CIPD_CLIENT=${DEPOT_TOOLS_HOME}/.cipd_client
export DEPOT_TOOLS_BOOTSTRAP_PYTHON3=0
export VPYTHON_BYPASS='manually managed python not supported by chrome operations'

if [ -d "venv" ]; then
    # virtual env exists, no need to create
    echo "venv exists"
else 
    # Create the virtual env in the v8port install dir
    echo "    Creating a python venv..."
    python3 -m venv venv
fi

# depot_tools creates the venv, here we ensure the necessary packages are installed
# Activate the path
echo "activating the virtual env - venv"
. venv/bin/activate

# Update the virtual env
echo "updating the virtual env pip"
python -m pip install -U pip


# Install the necessary requirements
if [ -d "venv" ]; then
    # virtual env exists, no need to install libs
    echo "venv exists"
else 
    # Install the necessary libs
    echo "Installing reqired package in venv"
    pip install -r requirements.txt
fi


echo "Invoking gclient metrics --opt-out"
# Comment out this line if you wish to participate.
gclient metrics --opt-out

echo "Change to v8base/v8 and run ninja"
echo "  eg: v8base/v8"
echo "      gn gen -v -c out/zos_s390x.release"
echo "      ninja -v -C out/zos_s390x.release"

