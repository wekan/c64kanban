if [[ "$OSTYPE" == "linux-gnu" ]]; then
	sudo apt -y install cc65
elif [[ "$OSTYPE" == "darwin"* ]]; then
	brew install cc65
fi
cl65 -t c64 kanban.c -o kanban.prg
