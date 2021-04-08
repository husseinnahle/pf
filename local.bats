#!/usr/bin/env bats

load test_helper

@test "gratuit" {
	true
}

@test "empty args" {
	run ./pf
	test "$status" = 127
	test "$output" != ""
}

@test "invalid option" {
	run ./pf -f ./work
	test "$status" = 127
	test "$output" != ""
}

@test "invalid args" {
	run ./pf -n 6 -u ./work
	test "$status" = 127
	test "$output" != ""
}

