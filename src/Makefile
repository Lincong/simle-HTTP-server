################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Recitation 1.             #
#                                                                              #
# Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                          #
#          Wolf Richter <wolf@cs.cmu.edu>                                      #
#                                                                              #
################################################################################

SERVER_SRC = echo_server.c utility.c message.c

default: echo_server echo_client

echo_server:
	@gcc $(SERVER_SRC) -o echo_server -Wall -Werror

echo_client:
	@gcc echo_client.c -o echo_client -Wall -Werror

clean:
	@rm echo_server echo_client
