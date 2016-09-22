#!/bin/bash
counter=0
while read -r line; do
	convert -define jpeg:size=100x100 -extent 100x100 -gravity center -thumbnail 100x100 $line thumbnail$counter
	counter=$counter+1
done < <(./vsmatch query_dataset train_dataset)
montage -geometry +0+0 thumbnail* result.jpg
rm thumbnail*
