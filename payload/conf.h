#pragma once

#define CAPTURE_PICAM "raspistill -w %d -h %d -n -t 10 -q 90 -e jpg -mm matrix -drc high -ifx denoise -o %s"
#define CAPTURE_V4L "avconv -f video4linux2 -s %dx%d -i /dev/video0 -frames 1 -f mjpeg -y %s"
