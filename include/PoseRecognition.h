#pragma once
#include "Types.h"
#include <opencv2/opencv.hpp>
#include <QObject>
#include <QVector>
#include <opennn.h>
class QFile;
namespace iez {

class PoseRecognition {
public:
	enum POSE {
		POSE_0 = 0,
		POSE_1 = 1,
		POSE_2 = 2,
		POSE_3 = 3,
		POSE_4 = 4,
		POSE_5 = 5,
		POSE_6 = 6,
		POSE_7 = 7,
//		POSE_8 = 8,
//		POSE_9 = 9,
//		POSE_10 = 10,
//		POSE_11 = 11,

		POSE_END = 8
	};

	PoseRecognition();

	void learnNew(const POSE pose,
			   const cv::Point palmCenter,
			   const float palmRadius,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);
	void savePoseDatabase(QString path);
	void loadPoseDatabase(QString path);
	void neuralNetworkImport(QString path);
	void neuralNetworkLoad(QString path);
	void neuralNetworkSave(QString path);
	void train();
	QString databaseToString() const;

	QString categorize(const cv::Point palmCenter,
					const float palmRadius,
					const wristpair_t &wrist,
					const QList<cv::Point> &fingertips) const;

	int calculateOutput(OpenNN::Vector<double> featureVector) const;

	bool testNeuralNetwork() const;

	struct Data {
		Data(int size)
		: input(size) {

		}

		QVector<float> input;
		int output;
	};

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

	static QString poseToString(enum POSE pose);
	static QString poseToString(int pose);
	static int inputVectorSize();
	static double normalizeInto(double value, double low = 0.0, double high = 0.0);
	static QList<Data> loadDatabaseFromFile(QString path);
	static void saveDatabaseToFile(QString path, QList<Data> database);
private:


	static bool loadLayer(QFile &fBias,
						  QFile &fWeights,
						  const int inputCount,
						  const int neuronCount,
						  OpenNN::PerceptronLayer &layer);
	static bool loadMLP(QString prefix, OpenNN::MultilayerPerceptron& mlp);




	QList<Data> m_database;

	static void normalizeVector(OpenNN::Vector<double> &vec);

	static OpenNN::Vector<double> constructFeatureVector(	const cv::Point palmCenter,
															const float palmRadius,
															const wristpair_t &wrist,
															const QList<cv::Point> &fingertips);

	static OpenNN::Matrix<double> convertToNormalizedMatrix(const QList<Data> &db);

	OpenNN::Matrix<double> m_matrix;
	OpenNN::NeuralNetwork *m_neuralNetwork;
};

}

