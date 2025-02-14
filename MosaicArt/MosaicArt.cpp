#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <string>

#define CV_INTER_AREA (3)
#define CV_CAP_PROP_POS_AVI_RATIO (2)
#define CV_CAP_PROP_FRAME_COUNT (7)
#define RGB_COLOR_NUM (3)
#define COLOR_DEPTH (1 << 3)
#define COLOR_NUM (1 << COLOR_DEPTH)

#define N 2

int helper[COLOR_NUM][COLOR_NUM][COLOR_NUM];

class data {
public:
	int index;
	std::vector<uchar> v;
	data() {
		v = std::vector<uchar>(RGB_COLOR_NUM * N * N);
	}
};

class vertex {
public:
	data val;
	vertex *left;
	vertex *right;
};

class axisSorter {
	int k;
public:
	axisSorter(int _k) : k(_k) {}
	double operator()(const data &a, const data &b) {
		return a.v[k] < b.v[k];
	}
};

double sq(double x) {
	return x * x;
}

class KDTree {
private:
	int dimension;
	vertex* root;
	double dist(const data &x, const data &y) {
		double ret = 0;
		for (int i = 0; i < dimension; ++i)
			ret += (x.v[i] - y.v[i]) * (x.v[i] - y.v[i]);
		return ret;
	}

	data query(data &a, vertex *v, int depth) {
		int k = depth % dimension;
		if ((v->left != nullptr && a.v[k] < v->val.v[k]) ||
			(v->left != nullptr && v->right == nullptr)) {
			data d = query(a, v->left, depth + 1);
			if (dist(v->val, a) < dist(d, a))
				d = v->val;
			if (v->right != nullptr && sq(a.v[k] - v->val.v[k]) < dist(d, a)) {
				data d2 = query(a, v->right, depth + 1);
				if (dist(d2, a) < dist(d, a))
					d = d2;
			}
			return d;
		}
		else if (v->right != nullptr) {
			data d = query(a, v->right, depth + 1);
			if (dist(v->val, a) < dist(d, a))
				d = v->val;
			if (v->left != nullptr && sq(a.v[k] - v->val.v[k]) < dist(d, a)) {
				data d2 = query(a, v->left, depth + 1);
				if (dist(d2, a) < dist(d, a))
					d = d2;
			}
			return d;
		}
		else
			return v->val;
	}

	vertex* makeKDTree(std::vector<data> &xs, int l, int r, int depth) {
		if (l >= r)
			return nullptr;

		sort(xs.begin() + l, xs.begin() + r, axisSorter(depth % dimension));
		int mid = (l + r) >> 1;

		vertex *v = new vertex();
		v->val = xs[mid];
		v->left = makeKDTree(xs, l, mid, depth + 1);
		v->right = makeKDTree(xs, mid + 1, r, depth + 1);

		return v;
	}
public:
	KDTree(std::vector<data> &xs, int r, int Dimension) : dimension(Dimension) {
		for (int i = 0; i < COLOR_NUM; ++i) {
			for (int j = 0; j < COLOR_NUM; ++j) {
				for (int k = 0; k < COLOR_NUM; ++k) {
					helper[i][j][k] = -1;
				}
			}
		}
		root = makeKDTree(xs, 0, r, 0);
	}

	int getIndex(data &a) {
		if (helper[a.v[0]][a.v[1]][a.v[2]] == -1) {
			int index = query(a, root, 0).index;
			helper[a.v[0]][a.v[1]][a.v[2]] = index;
			return index;
		}
		else {
			return helper[a.v[0]][a.v[1]][a.v[2]];
		}
	}
};

void printProgressBar(int max, int now) {
	if (!(now % (max / 50)) || now == max) {
		int t = now / (double)max * 50;
		std::cout << "\r[" << std::string(t, '#') << std::string(50 - t, ' ') << "]" << t * 2 << "%";
	}
}

int main(void) {
	std::string inputPass;
	std::string outputPass;
	inputPass = "in/";
	outputPass = "out/";
	std::string imageName;
	std::string fileName;
	imageName = "s.png";
	fileName = "s.mp4";
	int cols;
	int rows;
	double mag;
	int dis = 690000;

	cols = 200 * N;
	rows = 200 * N;
	mag = 120 / N * 0.75 / 1.5;

	cv::Mat loadImage = cv::imread(inputPass + imageName);
	cv::Size size(loadImage.cols, loadImage.rows);
	size.height *= mag;
	size.width *= mag;

	cv::Mat resultImage(size, CV_8UC3, cv::Scalar(0));
	cv::Mat bufImage;

	std::cout << "Mosaic art info\n";
	std::cout << "Size:" << size.width << "x" << size.height << "\n\n";

	std::cout << "Loading images...\n";

	cv::VideoCapture video(inputPass + fileName);

	int frameNum = video.get(CV_CAP_PROP_FRAME_COUNT) - dis;

	std::cout << "Frame count:" << frameNum << "\n";

	std::vector<data> xs(frameNum);
	std::vector<cv::Mat> images(frameNum);

	cv::Mat frame;
	video >> frame;

	double fx, fy;
	fx = 1.0 / frame.cols * size.width / cols * N;
	fy = 1.0 / frame.rows * size.height / rows * N;

	double fx2, fy2;
	fx2 = 1.0 / (size.width / cols);;
	fy2 = 1.0 / (size.height / rows);

	video.set(CV_CAP_PROP_POS_AVI_RATIO, 0);

	for (int i = 0; i < frameNum; ++i) {
		video >> frame;

		resize(frame, frame, cv::Size(), fx, fy);
		images[i] = frame;

		resize(frame, frame, cv::Size(), fx2, fy2, CV_INTER_AREA);

		data a;
		for (int x = 0; x < N; x++) {
			for (int y = 0; y < N; y++) {
				for (int c = 0; c < RGB_COLOR_NUM; c++) {
					a.v[(x * N + y) * RGB_COLOR_NUM + c] = frame.ptr<cv::Vec3b>(y)[x][c];
				}
			}
		}
		a.index = i;
		xs[i] = a;

		printProgressBar(frameNum - 1, i);
	}


	std::cout << "\n";
	std::cout << "Loading Images successed!\n\n";

	std::cout << "Building KD-Tree...\n";
	KDTree kd(xs, frameNum, RGB_COLOR_NUM * N * N);
	std::cout << "Building KD-Tree successed!\n\n";

	std::cout << "Creating mosaic art...\n";
	resize(loadImage, loadImage, cv::Size(), 1.0 / loadImage.cols * cols, 1.0 / loadImage.rows * rows, CV_INTER_AREA);
	for (int i = 0; i < loadImage.cols / N; i++) {
		for (int j = 0; j < loadImage.rows / N; j++) {
			data a;
			std::vector<uchar> v(N * N * RGB_COLOR_NUM);
			for (int x = 0; x < N; x++) {
				for (int y = 0; y < N; y++) {
					for (int c = 0; c < RGB_COLOR_NUM; c++) {
						a.v[(x * N + y) * RGB_COLOR_NUM + c] = loadImage.ptr<cv::Vec3b>( j * N + y)[ i * N + x][c];
					}
				}
			}
			bufImage = images[kd.getIndex(a)];
			bufImage.copyTo(cv::Mat(resultImage, cv::Rect(i * size.width / cols * N, j * size.height / rows * N, bufImage.cols, bufImage.rows)));
		}
		printProgressBar(loadImage.cols - 1, i);
	}
	std::cout << "\n";
	std::cout << "Creating a mosaic art succesed!\n\n";

	std::cout << "Writing a mosaic art...\n";
	std::string fileNameA = outputPass + imageName + "_cols" + std::to_string(cols) + "_N" + std::to_string(N) +  "_dis" + std::to_string(dis) + ".png";
	std::cout << "File pass: " + fileNameA << "\n";
	imwrite(fileNameA, resultImage);
	std::cout << "Writing a mosaic art succesed!\n";
	return 0;
}
