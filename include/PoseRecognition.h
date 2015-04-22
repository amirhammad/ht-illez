#pragma once
#include "Types.h"
#include <opencv2/opencv.hpp>
#include <QSettings>
#include <QObject>
#include <QVector>
#include <opennn.h>
class QFile;
namespace iez {

class PoseRecognition {
public:
	enum POSE {
		POSE_0 = 0,//
		POSE_1 = 1,//T
		POSE_2 = 2,//middle and pointing finger
		POSE_3 = 3,//POSE_2 + T
		POSE_4 = 4,// POSE_3 + ring
		POSE_5 = 5,// 5 fingers
		POSE_6 = 6,// 2_2_T
		POSE_7 = 7,//1_2_1_T
		POSE_8 = 8,// POSE_6 - T
//		POSE_9 = 9,// POSE_7 - T
//		POSE_10 = 10,// pinky_(ring,middle,point)
//		POSE_11 = 11,// POSE_10 + T

		POSE_END
	};

	PoseRecognition();

	void learnNew(const POSE pose,
			   const cv::Point palmCenter,
			   const float palmRadius,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);
	void savePoseDatabase();
	void neuralNetworkLoad(std::string path);
	void neuralNetworkSave(std::string path);
	void train();
	QString databaseToString() const;

	QString categorize(const cv::Point palmCenter,
					const float palmRadius,
					const wristpair_t &wrist,
					const QList<cv::Point> &fingertips);

	int calculateOutput(OpenNN::Vector<double> featureVector) const;

	bool testNeuralNetwork() const;
	static QString poseToString(enum POSE pose);
	static QString poseToString(int pose);

	struct Data {
		Data(int size)
		: input(size) {

		}

		QVector<float> input;
		int output;
	};
	static QList<Data> loadDatabaseFromFile(QString path);
	static void saveDatabaseToFile(QString path, QList<Data> database);
	static int inputVectorSize();
	static double normalizeInto(double value, double low = 0.0, double high = 0.0);

	template <typename Container>
	static int findBestMatchIndex(Container container, double target)
	{
		int minIndex = 0;
		double minValue = std::numeric_limits<double>::max();

		for (int i = 0; i < container.size(); i++) {
			double err = qAbs(target - container[i]);
			if (minValue > err) {
				minValue = err;
				minIndex = i;
			}
		}

		return minIndex;
	}

	static bool loadLayer(QFile &fBias,
						  QFile &fWeights,
						  const int inputCount,
						  const int neuronCount,
						  OpenNN::PerceptronLayer &layer);
	static bool loadMLP(QString prefix, OpenNN::MultilayerPerceptron& mlp);
private:

	QList<Data> m_database;

	static void normalizeVector(OpenNN::Vector<double> &vec);

	void teachNN();
	static OpenNN::Vector<double> constructFeatureVector(	const cv::Point palmCenter,
															const float palmRadius,
															const wristpair_t &wrist,
															const QList<cv::Point> &fingertips);
	void appendToMatrix(OpenNN::Vector<double> vec);

	static OpenNN::Matrix<double> convertToNormalizedMatrix(const QList<Data> &db);

	OpenNN::Matrix<double> m_matrix;
	OpenNN::NeuralNetwork *m_neuralNetwork;
	QSettings *m_settings;
};
}

