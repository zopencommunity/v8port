#!/bin/sh

# The path and environment variables are updated as a result of the package install.
# However, if the user installs the package and does not run the zopen-config
# command in the current shell, then the path will not be there yet.  In 
# this case check for the path in the current path.

desired_path="zopen/usr/local/zopen/depot_tools/depot_tools-main"

if  expr "${PATH}" : ".*${desired_path}.*" 1>/dev/null ; then
    echo "Good. depot_tools is in path."
else
    echo "Path to depot_tools missing.  Adding it to path.."
    export PATH=${PATH}:${HOME}/${desired_path}
fi


# activate the virtual env
echo "activating the virtual env - venv"
. ${HOME}/zopen/usr/local/zopen/v8/v8-DEV/venv/bin/activate

# update the virtual env
python -m pip install -U pip


# install the necessary requirements
echo "Installing packages via requirements.txt..."
pip install -r ${HOME}/zopen/usr/local/zopen/v8/v8-DEV/requirements.txt

# Using depot_tools disable metrics.
# Comment out this line if you wish to participate.
gclient metrics --opt-out


# provide a mechanism to resume work on v8 versus 
# ensure pristine.   Assume by default the user
# desires a pristine state and a fresh fetch.
RESUME=0
while [ $# -gt 0 ]; do
    case "$1" in
        -resume)
            echo "-resume switch used.  Skipping .gclient* delete"
            RESUME=1
            shift
            ;;
        -optionx)
            echo "option x used with value $2"
            echo "TODO: nothing to do yet"
            shift 2
            ;;
        *)
            echo "unknown option"
            echo "USAGE: $0 [-resume]"
            exit 1
            ;;
    esac
done

# remove any previous attempts of fetch
# so as to pull a fresh copy.  If this is the first time building
# depot tools, these file will not be present and thus
# no need to attempt to remove them. 
# NOTE: assuming v8 will be built in $HOME/zopen/dev/v8port/v8base
if [ ${RESUME} -eq 0 ]; then
    echo "Attempting to delete any leftover .gclient* entries"
    if [ -d "${HOME}/zopen/dev/v8port/v8base" ]; then
        if [ -f "${HOME}/zopen/dev/v8port/v8base/.gclient_entries" ]; then
            rm ${HOME}/zopen/dev/v8port/v8base/.gclient_entries
        fi
        if [ -f "${HOME}/zopen/dev/v8port/v8base/.gclient" ]; then
            rm ${HOME}/zopen/dev/v8port/v8base/.gclient
        fi
    fi
fi


# start ssh-agent
if ps -A | grep ssh-agent; then
    echo "ssh-agent is already running"
    echo "connecting to existing now..."
    AGENT_SCRIPT=$(ssh-agent -s)
    # Now run this script in current shell
    eval "${AGENT_SCRIPT}"

else
    echo "ssh-agent is not running"
    echo "starting now..."
    eval `ssh-agent`
fi


