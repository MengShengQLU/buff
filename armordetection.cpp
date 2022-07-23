#include "armordetection.h"
#include <opencv2/imgproc/types_c.h>

ArmorDetection::ArmorDetection() = default;

ArmorDetection::ArmorDetection(Mat & input) {
	frame = input;
}

void ArmorDetection::setInputImage(Mat input) {

	frame = input;

	currentCenter.x = 0;
	currentCenter.y = 0;
}

//图像预处理
void ArmorDetection::Pretreatment() {
	Mat input;
	Point p, center;
	vector<vector<Point>> contours;
	vector<Vec4i> hireachy;
	vector<Rect> boundRect(contours.size());
	Point2f vertex[4];

	//创建进度条


	cvtColor(frame, hsv, CV_BGR2HSV);
	inRange(hsv,
		Scalar(iLowH, iLowS, iLowV),
		Scalar(iHighH, iHighS, iHighV),
		mask);
	// 形态学操作
	morphologyEx(mask, mask, MORPH_OPEN, kernel1, Point(-1, -1));//开操作
	dilate(mask, mask, kernel2, Point(-1, -1), 1);//膨胀
	//轮廓增强
	Canny(mask, mask, 3, 9, 3);
   // imshow("mask",mask );
	findContours(mask, contours, hireachy, cv::RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//筛选，去除一部分矩形（删除单个灯条）
	for (int i = 0; i < contours.size(); ++i) {
		RotatedRect minRect = minAreaRect(Mat(contours[i]));
		minRect.points(vertex);
      //  cout<<"with"<<minRect.size.width<<"   "<<"height"<<minRect.size.height<<"angle"<<minRect.angle<<endl;
		if (minRect.size.width > minRect.size.height) {
            minRect.angle =min(abs(minRect.angle+ 90),abs(minRect.angle-90));
			float t = minRect.size.width;
			minRect.size.width = minRect.size.height;
			minRect.size.height = t;
		}
        //cout<<"with"<<minRect.size.width<<"   "<<"height"<<minRect.size.height<<"angle"<<minRect.angle<<endl;
		if ((minRect.size.width * 10 > minRect.size.height)
			&& (minRect.size.width * 1 < minRect.size.height)
            && ((abs(minRect.angle)) < 30)) {
			minRects.push_back(minRect);
		}
		for (int l = 0; l < 4; l++)
		{
			line(frame, vertex[l], vertex[(l + 1) % 4], Scalar(255, 0, 0), 2);
		}
		line(frame, Point(frame.cols / 2 - 15, frame.rows / 2),
			Point(frame.cols / 2 + 15, frame.rows / 2), Scalar(0, 255, 255), 5);
		line(frame, Point(frame.cols / 2, frame.rows / 2 - 15),
			Point(frame.cols / 2, frame.rows / 2 + 15), Scalar(0, 255, 255), 5);
		circle(frame, Point(frame.cols / 2, frame.rows / 2), 4, Scalar(0, 0, 255), -1);
	}
}

bool ArmorDetection:: GetArmorPnPdata(Point2f *Center, Point2f *tar) {
	//遍历所有矩形，两两组合
	RotatedRect leftRect, rightRect;
	vector<int*> reliability;
	double area[2], distance, height;
   // cout<<"3点"<<endl;
	if (minRects.size() < 2) {   
	//返回vecter大小并比较
        LostTarget();
       // cout<<minRects.size()<<endl;
         imshow("frame", frame);
        return    0;;
	}

	for (int i = 0; i < minRects.size(); ++i) {
		for (int j = i + 1; j < minRects.size(); ++j) {
			int level = 0;
			int temp[3];
			leftRect = minRects[i];
			rightRect = minRects[j];

			//判断
			if (leftRect.angle == rightRect.angle) {
				level += 10;
			}
			else if (abs(leftRect.angle - rightRect.angle) < 5) {
				level += 8;
			}
			else if (abs(leftRect.angle - rightRect.angle) < 10) {
				level += 6;
			}
			else if (abs(leftRect.angle - rightRect.angle) < 30) {
				level += 4;
			}
			else if (abs(leftRect.angle - rightRect.angle) < 90) {
				level += 1;
			}
			else {
				break;
			}

			area[0] = leftRect.size.width * leftRect.size.height;
			area[1] = rightRect.size.width * rightRect.size.height;
			if (area[0] == area[1]) {
				level += 10;
			}
			else if (min(area[0], area[1]) * 1.5 > max(area[0], area[1])) {
				level += 8;
			}
			else if (min(area[0], area[1]) * 2 > max(area[0], area[1])) {
				level += 6;
			}
			else if (min(area[0], area[1]) * 3 > max(area[0], area[1])) {
				level += 4;
			}
			else if (min(area[0], area[1]) * 4 > max(area[0], area[1])) {
				level += 1;
			}
			else {
				break;
			}

			double half_height = (leftRect.size.height + rightRect.size.height) / 4;
			if (leftRect.center.y == rightRect.center.y) {
				level += 10;
			}
			else if (abs(leftRect.center.y - rightRect.center.y) < 0.2 * half_height) {
				level += 8;
			}
			else if (abs(leftRect.center.y - rightRect.center.y) < 0.4 * half_height) {
				level += 6;
			}
			else if (abs(leftRect.center.y - rightRect.center.y) < 0.8 * half_height) {
				level += 4;
			}
			else if (abs(leftRect.center.y - rightRect.center.y) < half_height) {
				level += 1;
			}
			else {
				break;
			}

			distance = Distance(leftRect.center, rightRect.center);
			height = (leftRect.size.height + rightRect.size.height) / 2;
			if (distance != 0 && distance > height) {
				if (distance < 1.5 * height) {
					level += 6;
				}
				else if (distance < 1.8 * height) {
					level += 4;
				}
				else if (distance < 2.4 * height) {
					level += 2;
				}
				else if (distance < 10 * height) {
					level += 1;
				}
				else {
					break;
				}
			}

			temp[0] = i;
			temp[1] = j;
			temp[2] = level;

			reliability.push_back(temp);

		}
	}

	if (reliability.empty()) {
		LostTarget();
        return 0;
	}
	else {

		int maxLevel = 0, index = 0;
		for (int k = 0; k < reliability.size(); ++k) {
			if (reliability[k][2] > maxLevel) {
				maxLevel = reliability[k][2];
				index = k;
			}
		}

		currentCenter.x = (minRects[reliability[index][0]].center.x + minRects[reliability[index][1]].center.x) / 2;
		currentCenter.y = (minRects[reliability[index][0]].center.y + minRects[reliability[index][1]].center.y) / 2;
                //index[0]:右上角        index[1]:左上角
                float a = (float)minRects[reliability[index][1]].size.height ;//左侧灯条长
                float b = (float)minRects[reliability[index][0]].size.height ;//右侧灯条长
                float c = (float)minRects[reliability[index][1]].center.x ;//左 x
                float d = (float)minRects[reliability[index][0]].center.x ;//右 x
                float e = (float)currentCenter.y + (b/2);//右上角 y
                float f = (float)currentCenter.y - (b/2);//右下角 y
                float g = (float)currentCenter.y + (a/2);//左上角 y
                float h = (float)currentCenter.y - (a/2);//左下角 y
                //定义四个顶点并赋值
                Point2f light_up;//左上
                Point2f light_down;//左下
                Point2f right_up;//右上
                Point2f right_down;//右下

                light_up.x = c;
                light_up.y = h;
                right_up.x = d;
                right_up.y = f;
                right_down.x = d;
                right_down.y = e;
                light_down.x = c;
                light_down.y = g;

                //
                Center->x = currentCenter.x;
                Center->y = currentCenter.y;

                //输出四个点
//                cout<<light_up
                //reac
//                rectangle(frame,Point(c,h),Point(d,e),Scalar(0,0,255),2,8);
                //draw_line
//                line(frame,Point(c,h),Point(d,e),Scalar(0,0,255),3);
//                line(frame,Point(c,g),Point(d,f),Scalar(255,0,0),3);
                //draw_point
//                line(frame,Point(currentCenter.x - 10,currentCenter.y - 10),
//                     Point(currentCenter.x + 10,currentCenter.y + 10),Scalar(255,255,0),5);
//                line(frame,Point(currentCenter.x = 10,currentCenter.y - 10),
//                     Point(currentCenter.x - 10,currentCenter.y + 10),Scalar(255,255,0),5);
                circle(frame,light_up,7.5,Scalar(0,0,255),5);
                circle(frame,light_down,7.5,Scalar(0,0,255),5);
                circle(frame,right_up,7.5,Scalar(0,0,255),5);
                circle(frame,right_down,7.5,Scalar(0,0,255),5);
                circle(frame,currentCenter,7.5,Scalar(0,0,255),5);

		//与上一次的结果对比
		if (lastCenter.x == 0 && lastCenter.y == 0) {
			lastCenter = currentCenter;
			lost = 0;
		}
		else {
			double difference = Distance(currentCenter, lastCenter);
			if (difference > 300) {
				LostTarget();
                imshow("frame", frame);
                return 0;
			}
		}
		//绘制击打点准星
//		line(frame, Point(currentCenter.x - 10, currentCenter.y - 10),
//			Point(currentCenter.x + 10, currentCenter.y + 10), Scalar(255, 255, 0), 5);
//		line(frame, Point(currentCenter.x + 10, currentCenter.y - 10),
//			Point(currentCenter.x - 10, currentCenter.y + 10), Scalar(255, 255, 0), 5);
//		circle(frame, currentCenter, 7.5, Scalar(0, 0, 255), 5);

        tar[0]=light_up;
        tar[1]=right_up;
        tar[2]=right_down;
        tar[3]=light_down;
        imshow("frame", frame);
        return 1;
	}
}

void ArmorDetection::LostTarget() {
	lost++;
	if (lost < 3) {
		currentCenter = lastCenter;
	}
	else {
		currentCenter = Point2f(0, 0);
		lastCenter = Point2f(0, 0);
	}
}

double ArmorDetection::Distance(Point2f a, Point2f b) {
	return sqrt((a.x - b.x) * (a.x - b.x) +
		(a.y - b.y) * (a.y - b.y));
}

double ArmorDetection::max(double first, double second) {
	return first > second ? first : second;
}

double ArmorDetection::min(double first, double second) {
	return first < second ? first : second;
}


