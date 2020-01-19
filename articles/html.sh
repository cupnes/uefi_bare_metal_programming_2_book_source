#!/bin/bash

set -eu

IMG_WIDTH=600

PARSED_MATTER=()
PARSED_NAME=()
PARSED_TITLE=()
OUTDIR=html

usage() {
	(
		echo "Usage:"
		echo -e "\t$1 BOOK_TITLE"
	) 1>&2
	exit 1
}

if [ $# -ne 1 ]; then
	usage $0
fi

BOOK_TITLE=$1

parse() {
	chap_num=1
	appendix_num=65	# A
	while read line; do
		case $line in
		*:)
			matter=$(echo $line | cut -d':' -f1)
			;;
		*.re)
			name=$(echo $line | tr -d ' ' | cut -d'-' -f2- | rev | cut -d'.' -f2- | rev)
			title=$(grep '<title>' $name.html | head -n 1 | cut -d'>' -f2 | cut -d'<' -f1)
			case $matter in
			CHAPS)
				title="第${chap_num}章 $title"
				chap_num=$((chap_num + 1))
				;;
			APPENDIX)
				appendix_char=$(printf "%b" $(printf '%s%x' '\x' $appendix_num))
				title="付録${appendix_char} $title"
				appendix_num=$((appendix_num + 1))
			esac
			# echo "$matter,$name,$title" >>$PARSED_CSV
			PARSED_MATTER+=($matter)
			PARSED_NAME+=($name)
			PARSED_TITLE+=($title)
			;;
		esac
	done <catalog.yml
}

add_header_footer() {
	for ((i = 0; i < ${#PARSED_NAME[@]}; i++)); do
		if [ $i -eq 0 ]; then
			nav_prev_html="<div class=nav_prev style=display:table-cell;text-align:left>Prev</div>"
			nav_next_html="<div class=nav_next style=display:table-cell;text-align:right><a href=${PARSED_NAME[i+1]}.html>Next</a></div>"
		elif [ $((i + 1)) -eq ${#PARSED_NAME[@]} ]; then
			nav_prev_html="<div class=nav_prev style=display:table-cell;text-align:left><a href=${PARSED_NAME[i-1]}.html>Prev</a></div>"
			nav_next_html="<div class=nav_next style=display:table-cell;text-align:right>Next</div>"
		else
			nav_prev_html="<div class=nav_prev style=display:table-cell;text-align:left><a href=${PARSED_NAME[i-1]}.html>Prev</a></div>"
			nav_next_html="<div class=nav_next style=display:table-cell;text-align:right><a href=${PARSED_NAME[i+1]}.html>Next</a></div>"
		fi

		nav_html="<div class=navigator style=display:table;width:100%>${nav_prev_html}<div class=nav_name style=display:table-cell;text-align:center>${PARSED_TITLE[i]}</div>${nav_next_html}</div>"

		header_html="<hr><div style=text-align:center><a href=index.html>Top</a></div>${nav_html}<hr>"
		header_line_num=$(($(grep -n '<body>' ${PARSED_NAME[i]}.html | cut -d':' -f1) + 1))
		sed "${header_line_num}i${header_html}" ${PARSED_NAME[i]}.html > $OUTDIR/${PARSED_NAME[i]}.html

		footer_html="<hr>${nav_html}<div style=text-align:center><a href=index.html>Top</a></div><hr>"
		footer_line_num=$(grep -n '</body>' $OUTDIR/${PARSED_NAME[i]}.html | cut -d':' -f1)
		sed -i "${footer_line_num}i${footer_html}" $OUTDIR/${PARSED_NAME[i]}.html
	done
}

create_index() {
	cat <<EOF >$OUTDIR/index.html
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops" xmlns:ops="http://www.idpf.org/2007/ops" xml:lang="ja">
<head>
  <meta charset="UTF-8" />
  <link rel="stylesheet" type="text/css" href="style.css" />
  <meta name="generator" content="Re:VIEW" />
  <title>${BOOK_TITLE}</title>
</head>
<body>
<h1><a id="h"></a>${BOOK_TITLE}</h1>
<ul>
EOF

	for ((i = 0; i < ${#PARSED_NAME[@]}; i++)); do
		echo "<li><a href=${PARSED_NAME[i]}.html>${PARSED_TITLE[i]}</a></li>"
	done >>$OUTDIR/index.html

	cat <<EOF >>$OUTDIR/index.html
</ul>
<hr>
<div style=text-align:center><a href=http://yuma.ohgami.jp>筆者のウェブサイトへ戻る</a></div>
<hr>
</body>
</html>
EOF
}

put_images() {
	for ((i = 0; i < ${#PARSED_NAME[@]}; i++)); do
		for img in $(grep '<img' ${PARSED_NAME[i]}.html | cut -d'"' -f2); do
			# echo "${PARSED_NAME[i]}.html: $img"
			convert $img -resize ${IMG_WIDTH}x $OUTDIR/$img
		done
	done
}

old_ifs=$IFS
IFS=','
parse

# for ((i = 0; i < ${#PARSED_NAME[@]}; i++)); do
# 	echo "${PARSED_MATTER[i]},${PARSED_NAME[i]},${PARSED_TITLE[i]}"
# done

rm -rf $OUTDIR && mkdir -p $OUTDIR/images
cp style.css $OUTDIR

add_header_footer
create_index
IFS=${old_ifs}
put_images
