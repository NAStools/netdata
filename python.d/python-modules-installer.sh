#!/usr/bin/env bash

umask 022

dir="/usr/local/libexec/netdata/python.d"
target="${dir}/python_modules"
pv="$(python -V 2>&1)"

# parse parameters
while [ ! -z "${1}" ]
do
    case "${1}" in
    -p|--python)
        pv="Python ${2}"
        shift 2
        ;;

    -d|--dir)
        dir="${2}"
        target="${dir}/python_modules"
        echo >&2 "Will install python modules in: '${target}'"
        shift 2
        ;;

    -s|--system)
        target=
        echo >&2 "Will install python modules system-wide"
        shift
        ;;

    -h|--help)
        echo "${0} [--dir netdata-python.d-path] [--system]"
        echo "Please make sure you have installed packages: python-pip (or python3-pip) python-dev libyaml-dev libmysqlclient-dev"
        exit 0
        ;;

    *)
        echo >&2 "Cannot understand parameter: ${1}"
        exit 1
        ;;
    esac
done


if [ ! -z "${target}" -a ! -d "${target}" ]
then
    echo >&2 "Cannot find directory: '${target}'"
    exit 1
fi

if [[ "${pv}" =~ ^Python\ 2.* ]]
then
    pv=2
    pip="$(which pip2 2>/dev/null)"
elif [[ "${pv}" =~ ^Python\ 3.* ]]
then
    pv=3
    pip="$(which pip3 2>/dev/null)"
else
    echo >&2 "Cannot detect python version. Is python installed?"
    exit 1
fi

[ -z "${pip}" ] && pip="$(which pip 2>/dev/null)"
if [ -z "${pip}" ]
then
    echo >&2 "pip command is required to install python v${pv} modules."
    [ "${pv}" = "2" ] && echo >&2 "Please install python-pip."
    [ "${pv}" = "3" ] && echo >&2 "Please install python3-pip."
    exit 1
fi

echo >&2 "Working for python version ${pv} (pip command: '${pip}')"
echo >&2 "Installing netdata python modules in: '${target}'"

run() {
    printf "Running command:\n# "
    printf "%q " "${@}"
    printf "\n"
    "${@}"
}

# try to install all the python modules given as parameters
# until the first that succeeds
failed=""
installed=""
errors=0
pip_install() {
    local ret x msg="${1}"
    shift

    echo >&2
    echo >&2
    echo >&2 "Installing one of: ${*}"

    for x in "${@}"
    do
        echo >&2
        echo >&2 "attempting to install: ${x}"
        if [ ! -z "${target}" ]
        then
            run "${pip}" install --target "${target}" "${x}"
            ret=$?
        else
            run "${pip}" install "${x}"
            ret=$?
        fi
        [ ${ret} -eq 0 ] && break
        echo >&2 "failed to install: ${x}. ${msg}"
    done

    if [ ${ret} -ne 0 ]
    then
        echo >&2
        echo >&2
        echo >&2 "FAILED: could not install any of: ${*}. ${msg}"
        echo >&2
        echo >&2
        errors=$(( errors + 1 ))
        failed="${failed}|${*}"
    else
        echo >&2
        echo >&2
        echo >&2 "SUCCESS: we have: ${x}"
        echo >&2
        echo >&2
        installed="${installed} ${x}"
    fi
    return ${ret}
}

if [ "${pv}" = "2" ]
then
    pip_install "is libyaml-dev and python-dev installed?" pyyaml
    pip_install "is libmysqlclient-dev and python-dev installed?" mysqlclient mysql-python pymysql
else
    pip_install "is libyaml-dev and python-dev installed?" pyyaml
    pip_install "is libmysqlclient-dev and python-dev installed?" mysql-python mysqlclient pymysql
fi

echo >&2
echo >&2
if [ ${errors} -ne 0 ]
then
    echo >&2 "Failed to install ${errors} modules: ${failed}"
    if [ ! -z "${target}" ]
    then
		echo >&2
		echo >&2 "If you are getting errors during cleanup from pip, there is a known bug"
		echo >&2 "in certain versions of pip that prevents installing packages local to an"
		echo >&2 "application. To install them system-wide please run:"
		echo >&2 "$0 --system"
	fi
    exit 1
else
    echo >&2 "All done. We have: ${installed}"
    exit 0
fi
