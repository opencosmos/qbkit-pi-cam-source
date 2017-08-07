#pragma once

#define CAPTURE_PICAM "raspistill -w 800 -h 600 -n -t 10 -q 70 -e jpg -mm matrix -drc high -ifx denoise -o %s"
#define CAPTURE_V4L "avconv -f video4linux2 -s 640x480 -i /dev/video0 -frames 1 -f mjpeg -y %s"
