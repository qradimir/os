#! /usr/bin/env bash
echo TEST_1 ; echo -ne "ls" | ./simplesh ; echo -e "\n"
echo TEST_2 ; echo -ne "ls\nls" | ./simplesh ; echo -e "\n"
echo TEST_3 ; echo -ne "ls\nls\n" | ./simplesh ; echo -e "\n"
echo TEST_4 ; echo -ne "cat\nhello\n" | ./simplesh ; echo -e "\n"
echo TEST_5 ; { echo -ne "cat\nhe"; sleep 1; echo -ne "llo\n"; } | ./simplesh ; echo -e "\n"
echo TEST_6 ; { echo -ne "c"; sleep 1; echo -ne "at\nhello\n"; } | ./simplesh ; echo -e "\n"
echo TEST_7 ; echo -ne "cat | cat | grep hello | grep hi\nhello\nhi\nhello hi\nhihello\n" | ./simplesh ; echo -e "\n"
