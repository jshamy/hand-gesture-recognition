#include <iostream>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>

using namespace std;
using namespace cv;

POINT ptMouse;
Mat frame;

void drawRect(Mat& frame, Point center, int fingerCount) {
	string count = "finger:  ";
	int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
	double fontScale = 0.8;
	int thickness = 2;
	int baseline = 0;
	Size textSize = getTextSize(count, fontFace, fontScale, thickness, &baseline);

	//center the text
	Point textOrg1((center.x - textSize.width / 2.0), (center.y - textSize.height * 2.0));
	Point textOrg2((center.x - textSize.width / 2.0), (center.y - textSize.height * 4.0));

	putText(frame, "(" + to_string(center.x) + "," + to_string(center.y) + ")", textOrg1, fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
	putText(frame, count + to_string(fingerCount - 1), textOrg2, fontFace, fontScale, Scalar(0, 0, 255), thickness, 8);

}

void getFingerCount(const Mat& mask, Point center, double radius, double scale) {
	//�հ��� ������ ���� ���� �� �׸���
	Mat cImg(mask.size(), CV_8U, Scalar(0));
	circle(cImg, center, radius * scale, Scalar(255));

	//���� �ܰ����� ������ ����
	vector<vector<Point>> contours;
	findContours(cImg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	if (contours.size() == 0) //�ܰ����� ���� �� == �� ���� X
		return;

	//�ܰ����� ���� ���� mask�� ���� 0���� 1�� �ٲ�� ���� Ȯ��
	int fingerCount = 0;
	for (int i = 1; i < contours[0].size(); i++) {
		Point p1 = contours[0][i - 1];
		Point p2 = contours[0][i];
		if (mask.at<uchar>(p1.y, p1.x) == 0 && mask.at<uchar>(p2.y, p2.x) > 1)
			fingerCount++;
	}

	//�ո�� ������ ���� 1�� ����
	cout << "finger count: " << (fingerCount)-1 << endl;
	drawRect(frame, center, fingerCount);
}

Point getHandCenter(const Mat& mask, double& radius) {
	//�Ÿ� ��ȯ ����� ������ ����
	Mat dst;
	distanceTransform(mask, dst, DIST_L2, 5);  //����� CV_32SC1 Ÿ��

	//�Ÿ� ��ȯ ��Ŀ��� ��(�Ÿ�)�� ���� ū �ȼ��� ��ǥ��, ���� ���´�.
	int maxIdx[2];    //��ǥ ���� ���� �迭(��, �� ������ �����)
	minMaxIdx(dst, NULL, &radius, NULL, maxIdx, mask);   //�ּҰ��� ��� X

	return Point(maxIdx[1], maxIdx[0]);
}

int main(int argc, char** argv)
{
	VideoCapture cap;

	Mat ycrcb, channels[3];
	double Y, Cr, Cb;

	Mat skin_area;

	// Advance usage: select any API backend
	int deviceID = 0;     // 0 = Open default camera
	int apiID = CAP_ANY;  // 0 = Autodetect default API

	// ī�޶� ����.
	cap.open(deviceID + apiID);

	// ī�޶� ��ȿ���� Ȯ���Ѵ�.
	if (!cap.isOpened()) {
		cerr << "ERROR! Unable to open camera\n";
		return -1;
	}

	while (true) {
		// ����̽��κ��� ������ �о� frame�� �ִ´�.
		cap.read(frame);

		// ������ ��ȿ���� Ȯ���Ѵ�.
		if (frame.empty()) {
			cerr << "ERROR! blank frame grabbed\n";
			break;
		}

		// Hand Gesture Recognition
		cvtColor(frame, ycrcb, COLOR_BGR2YCrCb);
		split(ycrcb, channels);

		// 1) for�� ������ �ڵ��ϴ� ���
		/*int row = frame.rows;
		int col = frame.cols;
		skin_area = Mat(frame.rows, frame.cols, CV_8UC1, Scalar(0));
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				Y = (double)channels[0].at<uchar>(i, j);
				Cr = (double)channels[1].at<uchar>(i, j);
				Cb = (double)channels[2].at<uchar>(i, j);

				if ((Cr >= 133 && Cr <= 173) && (Cb >= 77 && Cb <= 127))
					skin_area.at<uchar>(i, j) = (uchar)255;
			}
		}*/

		// 2) API �Ἥ �ڵ��ϴ� ���
		inRange(ycrcb, Scalar(0, 133, 77), Scalar(255, 173, 127), skin_area);

		double radius;
		Mat element9(9, 9, CV_8U, Scalar(1));
		Point center = getHandCenter(skin_area, radius);
		circle(frame, center, 2, Scalar(0, 255, 0), -1);
		circle(frame, center, (int)(radius * 2.0), Scalar(255, 0, 0), 2);
		morphologyEx(skin_area, skin_area, MORPH_OPEN, element9);

		getFingerCount(skin_area, center, radius, 2.0);

		namedWindow("Live");
		HWND hWnd = FindWindow(NULL, L"Live");
		ptMouse.x = center.x;
		ptMouse.y = center.y;
		ClientToScreen(hWnd, &ptMouse);
		SetCursorPos(ptMouse.x, ptMouse.y);

		// ������ �ǽð����� ����Ѵ�.
		imshow("Live", frame);
		if (waitKey(50) >= 0)
			break;
	}

	return 0;
}
