#!/bin/bash

make && sudo make install && (sudo rmmod xylo_led; sudo modprobe xylo_led)
