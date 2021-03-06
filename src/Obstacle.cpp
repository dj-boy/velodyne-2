#include "Obstacle.h"
#include <math.h>
#include <list>
#include <iostream>
#include <opencv2/imgcodecs.hpp>

LidarObstacle::LidarObstacle(Cluster *cluster, odcore::data::TimeStamp current_time, uint64_t id) : clusterCandidates(), m_filter(), m_width(), m_length() {
    m_latestTimestamp = current_time;
    m_state << cluster->m_center[0], cluster->m_center[1], 0;
    m_mean_x = cluster->m_center[0];
    m_mean_y = cluster->m_center[1];
    m_initial_id = id;
    m_rectangle_center << 0, 0;
    m_movement_vector << 0, 0;
    m_movement_vector_filtered << 0, 0;
    m_filter.init(cluster->m_center[0], cluster->m_center[1], 0, 0, 0);
    m_rectangle[0] = Eigen::Vector2f();
    m_rectangle[1] = Eigen::Vector2f();
    m_rectangle[2] = Eigen::Vector2f();
    m_rectangle[3] = Eigen::Vector2f();
}

bool LidarObstacle::confidenceIsZero() {
    return m_confidence == 0;
}

double LidarObstacle::getDt(odcore::data::TimeStamp current_time) {
    return (current_time - m_latestTimestamp).toMicroseconds() / 1000000.0d;
}


void LidarObstacle::updateRectangle() {

    Eigen::Rotation2D<float> rot(-m_state[2]);
    std::list<Eigen::Vector2f> points;
    std::list<Eigen::Vector2f> points_mean;


    for (auto &cluster : clusterCandidates) {
        for (auto &point : cluster->m_cluster) {
            if (point->getZ() > m_max_height) {
                m_max_height = point->getZ();
            }
            Eigen::Vector2f tmp;
            tmp << point->getX(), point->getY();
            points.push_back(rot.toRotationMatrix() * tmp);
        }
    }


    float min_x = points.front()[0];
    float max_x = points.back()[0];
    float min_y = points.front()[1];
    float max_y = points.front()[1];
    for (auto &vec : points) {
        if (vec[1] < min_y)
            min_y = vec[1];
        if (vec[1] > max_y)
            max_y = vec[1];
        if (vec[0] > max_x)
            max_x = vec[0];
        if (vec[0] < min_x)
            min_x = vec[0];
    }

    float dxdd = (max_x - min_x) / 3.0f;
    float x1 = min_x + dxdd;
    float x2 = min_x + dxdd * 2;

    Eigen::MatrixXf Rect(2, 4);
    float thetaCorrection = 0;

//        if(m_initial_id == 93) {
//
//            const static int zoom = 48;
//            int res_x = int((max_x - min_x) * zoom);
//            int res_y = int((max_y - min_y) * zoom);
//            cv::Mat image(res_y, res_x, CV_8UC3, cv::Scalar(0, 0, 0));
//
//            for (auto &point : points) {
//                int x = static_cast<int>((point[0] - min_x) * zoom);
//                int y = res_y - static_cast<int>((point[1] - min_y) * zoom);
//
//                //std::cout << "Size: " << res_x << " : " << res_y << std::endl;
//                //std::cout << "X: " << x << std::endl;
//                //std::cout << "Y: " << y << std::endl;
//                if ((x < res_x) && (y < res_y) && (y >= 0) && (x >= 0)) {
//                    image.at<cv::Vec3b>(y, x)[0] = 255;
//                    image.at<cv::Vec3b>(y, x)[1] = 255;
//                    image.at<cv::Vec3b>(y, x)[2] = 255;
//                }
//            }
//
//            std::stringstream ss;
//            ss << "../images/Obst" << m_initial_id << "_" << img_count++ << ".bmp";
//            cv::imwrite(ss.str(), image);
//        }

    float p1_x, p1_y, p2_x, p2_y;
    if ((min_y + max_y) / 2 > 0) {
        // minimize y if are above the origin

        p1_x = 1000;
        p1_y = 1000;
        p2_x = 1000;
        p2_y = 1000;

        for (auto &vec : points) {
            if (vec[0] < x1) {
                if (vec[1] < p1_y) {
                    p1_x = vec[0];
                    p1_y = vec[1];
                }
            }
            if (vec[0] > x2) {
                if (vec[1] < p2_y) {
                    p2_x = vec[0];
                    p2_y = vec[1];
                }

            }
        }


        thetaCorrection = std::atan2(p2_y - p1_y, p2_x - p1_x);
        Eigen::Rotation2D<float> rotCorrection(-thetaCorrection);
        for (auto &point : points) {
            point = rotCorrection.toRotationMatrix() * point;
        }


        min_x = points.front()[0];
        max_x = points.back()[0];
        min_y = points.front()[1];
        max_y = points.front()[1];
        for (auto &vec : points) {
            if (vec[1] < min_y)
                min_y = vec[1];
            if (vec[1] > max_y)
                max_y = vec[1];
            if (vec[0] > max_x)
                max_x = vec[0];
            if (vec[0] < min_x)
                min_x = vec[0];
        }


        float width = getMostPropWidth(max_y - min_y);
        float lenght = getMostPropLenght(max_x - min_x);

        m_best_length = lenght;
        m_best_width = width;

        if ((min_x + max_x) / 2 > 0) {
            Rect(0, 0) = min_x;
            Rect(1, 0) = min_y;

            Rect(0, 1) = min_x;
            Rect(1, 1) = min_y + width;

            Rect(0, 2) = min_x + lenght;
            Rect(1, 2) = min_y + width;

            Rect(0, 3) = min_x + lenght;
            Rect(1, 3) = min_y;

        } else {
            Rect(0, 0) = max_x - lenght;
            Rect(1, 0) = min_y;

            Rect(0, 1) = max_x;
            Rect(1, 1) = min_y;

            Rect(0, 2) = max_x;
            Rect(1, 2) = min_y + width;

            Rect(0, 3) = max_x - lenght;
            Rect(1, 3) = min_y + width;
        }


    } else {
        // maximize y if are under the origin
        p1_x = -1000;
        p1_y = -1000;
        p2_x = -1000;
        p2_y = -1000;

        for (auto &vec : points) {
            if (vec[0] < x1) {
                if (vec[1] > p1_y) {
                    p1_x = vec[0];
                    p1_y = vec[1];
                }
            }
            if (vec[0] > x2) {
                if (vec[1] > p2_y) {
                    p2_x = vec[0];
                    p2_y = vec[1];
                }

            }
        }


        thetaCorrection = std::atan2(p2_y - p1_y, p2_x - p1_x);

        Eigen::Rotation2D<float> rotCorrection(-thetaCorrection);
        for (auto &point : points) {
            point = rotCorrection.toRotationMatrix() * point;
        }

        if (m_initial_id == 93) {
            std::cout << "------ thetaCorrection: " << thetaCorrection << std::endl;
        }


        min_x = points.front()[0];
        max_x = points.back()[0];
        min_y = points.front()[1];
        max_y = points.front()[1];
        for (auto &vec : points) {
            if (vec[1] < min_y)
                min_y = vec[1];
            if (vec[1] > max_y)
                max_y = vec[1];
            if (vec[0] > max_x)
                max_x = vec[0];
            if (vec[0] < min_x)
                min_x = vec[0];
        }


        float width = getMostPropWidth(max_y - min_y);
        float lenght = getMostPropLenght(max_x - min_x);
        m_best_length = lenght;
        m_best_width = width;

        if ((min_x + max_x) / 2 > 0) {

            Rect(0, 0) = min_x;
            Rect(1, 0) = max_y;

            Rect(0, 1) = min_x + lenght;
            Rect(1, 1) = max_y;

            Rect(0, 2) = min_x + lenght;
            Rect(1, 2) = max_y - width;

            Rect(0, 3) = min_x;
            Rect(1, 3) = max_y - width;
        } else {

            Rect(0, 0) = max_x - lenght;
            Rect(1, 0) = max_y - width;

            Rect(0, 1) = max_x;
            Rect(1, 1) = max_y - width;

            Rect(0, 2) = max_x;
            Rect(1, 2) = max_y;

            Rect(0, 3) = max_x - lenght;
            Rect(1, 3) = max_y;

        }


    }


    Eigen::Rotation2D<float> rotBack(m_state[2] + thetaCorrection);
    m_rectRot_old = m_rectRot;
    m_rectRot = fmod((m_state[2] + thetaCorrection + M_PI), (2.0 * M_PI)) - M_PI;


    Rect = rotBack.toRotationMatrix() * Rect;


    m_rectangle[0] = Eigen::Vector2f(Rect(0, 0), Rect(1, 0));
    m_rectangle[1] = Eigen::Vector2f(Rect(0, 1), Rect(1, 1));
    m_rectangle[2] = Eigen::Vector2f(Rect(0, 2), Rect(1, 2));
    m_rectangle[3] = Eigen::Vector2f(Rect(0, 3), Rect(1, 3));


    m_rectangle_center[0] = (Rect(0, 0) + Rect(0, 1) + Rect(0, 2) + Rect(0, 3)) / 4.0f;
    m_rectangle_center[1] = (Rect(1, 0) + Rect(1, 1) + Rect(1, 2) + Rect(1, 3)) / 4.0f;


}

void LidarObstacle::refresh(double movement_x, double movement_y, odcore::data::TimeStamp current_time, int img_count) {
    double dt = getDt(current_time);
    m_latestTimestamp = current_time;

    if (clusterCandidates.size() > 0) {

        m_mean_x = 0;
        m_mean_y = 0;
        double values_num = 0;


        for (auto &cluster : clusterCandidates) {
            values_num += cluster->getSize();
            for (auto &point : cluster->m_cluster) {
                m_mean_x += point->getX();
                m_mean_y += point->getY();
            }
        }
        m_mean_x /= values_num;
        m_mean_y /= values_num;


        m_state[0] += movement_x;
        m_state[1] += movement_y;


        float dx = m_mean_x - m_state[0];
        float dy = m_mean_y - m_state[1];


        float movement = std::sqrt(dx * dx + dy * dy);

        if (movement > 0 || !initial_theta) {
            initial_theta = true;
            m_state[0] = m_mean_x;
            m_state[1] = m_mean_y;
            m_state[2] = std::atan2(dy, dx);
        }


        m_movement_vector[0] = std::cos(m_state[2]) * 5 + m_mean_x;
        m_movement_vector[1] = std::sin(m_state[2]) * 5 + m_mean_y;


        float oldPosX = m_rectangle_center[0];
        float oldPosY = m_rectangle_center[1];

        updateRectangle();


        double speed = 0;
        if (oldPosX != 0 || oldPosY != 0) {
            m_speed_x = 0.5 * ((m_rectangle_center[0] - movement_x - oldPosX) / dt) + m_speed_x * 0.5;
            m_speed_y = 0.5 * ((m_rectangle_center[1] - movement_y - oldPosY) / dt) + m_speed_y * 0.5;
            speed = std::sqrt(m_speed_x * m_speed_x + m_speed_y * m_speed_y);
            if (speed > 100) {
                speed = 0;
            }

        }

        clusterCandidates.clear();

        double yaw_rate = (M_PI - std::fabs(std::fmod(std::fabs(m_rectRot_old - m_rectRot), 2.0 * M_PI) - M_PI));
        if ((m_rectRot_old + yaw_rate - m_rectRot) > 0.00001) {
            yaw_rate = -yaw_rate;
        }
        yaw_rate /= dt;

        m_filter.update(m_rectangle_center[0], m_rectangle_center[1], m_rectRot, speed, yaw_rate, movement_x, movement_y);


        m_movement_vector_filtered[1] = std::sin(m_filter.m_x[2]) * m_filter.m_x[3];
        m_movement_vector_filtered[0] = std::cos(m_filter.m_x[2]) * m_filter.m_x[3];
        m_movement_vector_filtered[0] += m_filter.m_x[0];
        m_movement_vector_filtered[1] += m_filter.m_x[1];


        if (speed > 10)
            m_confidence /= 2;


        // 0 -unclassified ; 1 - Car ; 2 cycelist ; 3 - pedestrian
        int type = 0;
        if (m_best_length <= 1.5 && m_best_width <= 1.5) {
            type = 3;
        } else if (m_best_length <= 2 && m_best_width <= 1.5) {
            type = 2;
        } else if (m_best_length <= 7 && m_best_width <= 4) {
            type = 1;
        }

        m_best_type = type;
        //std::cout << "Obst: "<<m_initial_id << std::endl;
        //std::cout << "width: " << m_best_width << std::endl;
        //std::cout << "length: " << m_best_length << std::endl;

        if (m_max_height > 4.5) {//building
            m_confidence /= 2;
            type = 4;
        } else if (m_best_width > 4 || m_best_length > 7) {
            //std::cout << "Confidence Wrong size! "  << std::endl;
            m_confidence /= 2;
        } else m_confidence++;


        if (m_initial_id == 54) {
            std::cout << "------------------------" << std::endl;
            std::cout << "dt: " << dt << std::endl;
            std::cout << "ID: " << m_initial_id << std::endl;
//            std::cout << "Dx: " << dx << std::endl;
//            std::cout << "Dy: " << dy << std::endl;
//            std::cout << "movement_x: " << movement_x << std::endl;
//            std::cout << "movement_y: " << movement_y << std::endl;
            std::cout << "m_current_mean_x: " << m_rectangle_center[0] << std::endl;
            std::cout << "m_current_mean_y: " << m_rectangle_center[1] << std::endl;
            std::cout << "kalman_rot: " << m_filter.m_x[2] / M_PI * 180 << std::endl;
            std::cout << "m_rectRot: " << m_rectRot / M_PI * 180 << std::endl;
            std::cout << "Theta: " << m_state[2] / M_PI * 180 << std::endl;
            std::cout << "speed: " << speed << std::endl;
            std::cout << "m_best_width: " << m_best_width << std::endl;
            std::cout << "m_best_length: " << m_best_length << std::endl;
            std::cout << "yawrate: " << yaw_rate << std::endl;
        }

        m_confidence++;
    } else {
        m_confidence /= 2;
    }


}


double LidarObstacle::getDistance(Cluster &cluster) {
    return std::sqrt(
            (cluster.m_center[0] - m_state[0]) * (cluster.m_center[0] - m_state[0]) + (cluster.m_center[1] - m_state[1]) * (cluster.m_center[1] - m_state[1]));
}


bool LidarObstacle::isInRect(Point &point) {
    Eigen::Rotation2D<float> rotCorrection(-m_rectRot);
    Eigen::Vector2f rrect[4];
    for (int i = 0; i < 4; i++) {
        rrect[i] = rotCorrection.toRotationMatrix() * m_rectangle[i];
    }
    return true;
}




