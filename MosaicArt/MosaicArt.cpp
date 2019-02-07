#include "pch.h"
#include <iostream>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iomanip>

#define CV_INTER_AREA 3
#define CV_CAP_PROP_POS_AVI_RATIO 2
#define CV_CAP_PROP_FRAME_COUNT 7

int helper[256][256][256];

class data {
public:
	int index;
	cv::Vec3b v;
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
		for (int i = 0; i < 256; ++i)
			for (int j = 0; j < 256; ++j)
				for (int k = 0; k < 256; ++k)
					helper[i][j][k] = -1;
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
		int t = now / ((double)max / 50);
		std::cout << "\r[" << std::string(t, '#') << std::string(50 - t, ' ') << "]" << t * 2 << "%";
	}
}

int main(void) {

	// 読み込むファイルのファイル名
	std::string movieName;
	std::string imageName;

	int cols;
	int rows;
	int mag;
	int dis = 1;

	movieName = "s.mp4";
	imageName = "s.jpg";
	cols = 250;
	rows = 250;
	mag = 40;

	// 動画・画像を読み込む
	cv::VideoCapture video(movieName);
	cv::Mat loadImage = cv::imread(imageName);

	// 出力する画像の解像度を設定
	cv::Size size(loadImage.cols, loadImage.rows);
	size.height *= mag;
	size.width *= mag;

	int frameNum = video.get(CV_CAP_PROP_FRAME_COUNT) / dis;

	std::vector<data> xs(frameNum);
	cv::Mat frame;

	cv::Mat resultImage(size, CV_8UC3, cv::Scalar(0));
	cv::Mat bufImage;

	std::cout << "出力する画像の情報\n";
	std::cout << "・画素数:" << size.width << "*" << size.height << "\n\n";

	std::cout << "総フレーム数:" << frameNum << "\n";
	std::cout << "ファイルの読み込みを開始します\n";

	// フレームを取得する
	video >> frame;

	// リサイズする比率を設定
	double fx, fy;
	fx = 1.0 / frame.cols * size.width / cols;
	fy = 1.0 / frame.rows * size.height / rows;

	// 平均の色用
	double fx2, fy2;
	fx2 = 1.0 / (size.width / cols);
	fy2 = 1.0 / (size.width / rows);

	cv::Mat* images = new cv::Mat[frameNum];

	// 参照先を最初のフレームに戻す
	video.set(CV_CAP_PROP_POS_AVI_RATIO, 0);

	for (int i = 0; i < frameNum; ++i) {
		// フレームを取得する
		video >> frame;

		// 出力時に使用する画質にリサイズ
		resize(frame, frame, cv::Size(), fx, fy);
		images[i] = frame;

		// 平均の色を求める
		resize(frame, frame, cv::Size(), fx2, fy2, CV_INTER_AREA);

		// ポインタを使って色を取得する
		xs[i].v = frame.ptr<cv::Vec3b>(0)[0];
		xs[i].index = i;

		printProgressBar(frameNum - 1, i);
	}
	std::cout << "\n";
	std::cout << "ファイルの読み込みが完了しました\n\n";

	// KD木の構築
	std::cout << "KD木を構築しています…\n";
	KDTree kd(xs, frameNum, 3);
	std::cout << "KD木の構築が完了しました\n\n";

	// モザイクアートの作成
	std::cout << "モザイクアートを作成します…\n";
	resize(loadImage, loadImage, cv::Size(), 1.0 / loadImage.cols * cols, 1.0 / loadImage.rows * rows, CV_INTER_AREA);
	for (int i = 0; i < loadImage.cols; i++) {
		for (int j = 0; j < loadImage.rows; j++) {
			data a;
			a.v = loadImage.ptr<cv::Vec3b>(j)[i];
			bufImage = images[kd.getIndex(a)];
			bufImage.copyTo(cv::Mat(resultImage, cv::Rect(i * size.width / cols, j * size.height / rows, bufImage.cols, bufImage.rows)));
		}
		printProgressBar(loadImage.cols - 1, i);
	}
	std::cout << "\n";
	std::cout << "モザイクアートの作成が完了しました\n\n";

	// 画像の書き出し
	std::cout << "画像を書き出しています…\n";
	std::string fileName = "out/" + imageName + ".png";
	imwrite(fileName, resultImage);
	std::cout << "画像を書き出しが完了しました\n";
	return 0;
}
