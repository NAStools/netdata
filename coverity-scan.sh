#/bin/bash

token=
[ -f .coverity-token ] && token="$(<.coverity-token)"
[ -z "${token}" ] && \
	echo >&2 "Save the coverity token to .coverity-token" && \
	exit 1

echo >&2 "Coverity token: ${token}"

covbuild="$(which cov-build 2>/dev/null || command -v cov-build 2>/dev/null)"
[ -z "${covbuild}" -a -f .coverity-build ] && covbuild="$(<.coverity-build)"
[ -z "${covbuild}" ] && \
	echo "Save command the full filename of cov-build in .coverity-build" && \
	exit 1

[ ! -x "${covbuild}" ] && \
	echo "The command ${covbuild} is not executable. Save command the full filename of cov-build in .coverity-build" && \
	exit 1

echo >&2 "Cleaning up old builds..."
make clean || exit 1

[ -d "cov-int" ] && \
	rm -rf "cov-int"

[ -f netdata-coverity-analysis.tgz ] && \
	rm netdata-coverity-analysis.tgz

"${covbuild}" --dir cov-int make -j4 || exit 1

tar czvf netdata-coverity-analysis.tgz cov-int || exit 1

curl --form token="${token}" \
  --form email=costa@tsaousis.gr \
  --form file=@netdata-coverity-analysis.tgz \
  --form version="1.3.0rc1" \
  --form description="Description" \
  https://scan.coverity.com/builds?project=firehol%2Fnetdata
