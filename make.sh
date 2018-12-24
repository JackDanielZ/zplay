gcc -g zplay.c $CFLAGS `pkg-config --cflags --libs elementary` -o zplay
[ $? -eq 0 ] || exit 1
sudo /usr/bin/install -c zplay /opt/e/bin/
