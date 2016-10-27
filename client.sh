#!/bin/sh

sudo ./c1218_stack -t client -d /dev/ttyS1 2>&1 | tee log.client
