make && valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all  --track-fds=yes build/evilwebserver
