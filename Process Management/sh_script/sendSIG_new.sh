if ! kill -"$1" "$2" > /dev/null 2>&1; then
    echo "1" >&2
else
	echo "0"
fi

