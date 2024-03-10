file_path="/tmp/.directorytmp"

./navigator
if test -e "$file_path"; then
	directory_path=$(<"$file_path")
	cd "$directory_path"
fi
