#include <iostream>
#include "tracker/tracker_base.h"
#include "common/drawer.h"
#include "common/parser.h"
#include "common/imu_reader.h"
#include "common/img_reader.h"
#include "camera/camera_base.h"
#include "common/geometry_utility.h"
#include "common/timer.h"
#include "sfm/frame.h"
#include "sfm/point.h"
#include "sfm/sfm.h"
#include "sfm/viewer.h"
#include "imu/preintegrator.h"
#include "yavie/rotation_calib.h"

using namespace hityavie;

int main(int argc, char **argv) {
    const std::string cfg_folder(argv[1]);
    const std::string data_folder(argv[2]);
    const std::string imu_file(data_folder + "/imu.txt");
    const std::string img_root_dir(data_folder);
    const std::string img_file(data_folder + "/images.txt");
    const int start_idx = 928;
    const int end_idx = 1820;
    YavieParameter param;
    Parser::ParsePrototxt<YavieParameter>(cfg_folder + "/param.prototxt", param);
    ImuReader imu_reader;
    imu_reader.Init(imu_file);
    const double img_timeoff = -0.0166845720919;
    ImgReader img_reader;
    img_reader.Init(img_file, img_root_dir, img_timeoff);
    BaseCamera::Ptr cam;
    cam.reset(BaseCameraRegisterer::GetInstanceByName(param.cam().type()));
    cam->Init(param.cam());
    BaseTracker::Ptr tracker;
    tracker.reset(BaseTrackerRegisterer::GetInstanceByName(param.tp().type()));
    tracker->Init(param.tp(), cam);
    Sfm::Ptr sfm(new Sfm(cam, param.sp()));
    RotationCalib::Ptr calib(new RotationCalib());
    // Viewer::Ptr viewer(new Viewer());
    // viewer->InitViewer(sfm->GetMap());
    Eigen::Vector3d gravity;
    gravity << 0, 0, 9.81;
    Timer timer;
    std::map<int, Eigen::Vector2d> id_position_map;
    double last_img_timestamp;

    for (int img_idx = start_idx; img_idx <= end_idx; ++img_idx) {
        std::cout << "Img idx " << img_idx << std::endl;
        double img_timestamp = img_reader.GetTimestamp4Id(img_idx);
        cv::Mat img = img_reader.GetImg4Id(img_idx);
        std::vector<Feature> kpts;
        std::vector<Feature> kpts_old;
        std::vector<Feature> kpts_new;
        std::vector<Eigen::Vector2d> pts_prev;
        std::vector<Eigen::Vector2d> pts_cur;

        timer.Tic();
        bool feat_status = tracker->Track(img, kpts);
        timer.Toc("feature tracking");
        timer.Tic();
        Frame::Ptr nf(new Frame(kpts));
        sfm->PushFrame(nf);
        timer.Toc("sfm");
        timer.Tic();
        if (sfm->GetState() != kSfmTracking && !calib->IsDone()) {
            Preintegrator::Ptr pitor(new Preintegrator());
            std::vector<ImuData> imu_data = imu_reader.GetImuDataBetweenImages(last_img_timestamp, img_timestamp);
            Eigen::Vector3d acc_init = imu_data[0].lin_acc;
            Eigen::Vector3d gyr_init = imu_data[0].ang_vel;
            pitor->Init(param.np(), gravity, acc_init, gyr_init, Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero());
            for (const auto &id : imu_data) {
                pitor->Integrate(id.timestamp - last_img_timestamp, id.lin_acc, id.ang_vel);
            }
            calib->Calibrate(sfm->GetRelativePose().block(0, 0, 3, 3), pitor->GetQ().toRotationMatrix());
            if (calib->IsDone()) {
                std::cout << "Calibrated ric: " << std::endl << calib->GetRic() << std::endl;
            }
        }
        timer.Toc("imu integration");

        for (const auto &pt : kpts) {
            if (id_position_map.count(pt.id)) {
                kpts_old.push_back(pt);
                pts_prev.push_back(id_position_map[pt.id]);
                pts_cur.push_back(pt.pt_ori);
                id_position_map[pt.id] = pt.pt_ori;
            } else {
                kpts_new.push_back(pt);
                id_position_map[pt.id] = pt.pt_ori;
            }
        }
        cv::cvtColor(img, img, CV_GRAY2BGR);
        Drawer::DrawPts(img, kpts_old, true, cv::Scalar(0, 255, 0));
        Drawer::DrawPts(img, kpts_new, true, cv::Scalar(0, 0, 255));
        Drawer::DrawPtsTraj(img, pts_prev, pts_cur, cv::Scalar(0, 255, 0));
        cv::imshow("img", img);
        cv::waitKey(1);
        last_img_timestamp = img_timestamp;
    }

    return 0;
}