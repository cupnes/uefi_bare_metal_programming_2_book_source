#!/bin/bash

set -ue

BOOK_TITLE="フルスクラッチで作る!UEFIベアメタルプログラミング パート2"

usage() {
	(
		echo "Usage:"
		echo -e "\t$1 OUTPUT_FILE_TYPE"
		echo
		echo -e "\tOUTPUT_FILE_TYPE\t \"pdf\" or \"html\""
	) 1>&2
	exit 1
}

if [ $# -ne 1 ]; then
	usage $0
fi

case $1 in
"pdf" | "html")
	type=$1
	;;
*)
	usage $0
esac

sudo chown -R $(id -u):$(id -g) .

docker run -t --rm -v $(pwd):/book yohgami/review /bin/bash -ci \
       "cd /book && ./setup.sh && npm run ${type}"

sudo chown -R $(id -u):$(id -g) .

if [ "${type}" = "html" ]; then
	cd articles/ && ./html.sh "${BOOK_TITLE}"
fi
