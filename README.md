# hityavie
Yet another visual inertial estimator(YAVIE). 

This is a hands on project for fun. I learned a lot and got a lot of fun while creating it. Hope you'll enjoy it as I do.

## Structrue from motion
See folder sfm. This folder contains a monocular visual odometry module which can run with only image infomation.

## Visual inertial odometry
See folder yavie. We don't use sliding window(schur complement) to limit the computation. Instead the double window optimization(see ORB-SLAM for details) is used here. There maybe many bugs in this code. The way we utilize imu information is borrowed shamelessly from VINS-MONO. Currently loop closing is not contained in the repo.

## Some demo images
Initialization success. See pictures/init.png.

Tracking. See pictures/tracking.png, pictures/tracking2.png.

Blue boxes are trajectories of cameras. Purple points are map points while the green are currently observed points.


## Test
```
# Dowload datset
wget http://rpg.ifi.uzh.ch/datasets/uzh-fpv-newer-versions/v2/indoor_forward_3_davis_with_gt.zip
unzip indoor_forward_3_davis_with_gt.zip
./Test ../proto/ indoor_forward_3_davis_with_gt/
```