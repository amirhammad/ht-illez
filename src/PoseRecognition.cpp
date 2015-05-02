/*
 * This file is part of the project HandTrackerApp - ht-illez
 * hosted at http://github.com/amirhammad/ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "PoseRecognition.h"
#include "Util.h"

#include <opennn.h>
#include <container.h>
#include <QVector>
#include <QFile>
#include <QDebug>
#include <QStringList>

#define NN_INPUT_VECTOR_SIZE 10

#define BOUND_X_LOW (0)
#define BOUND_X_HIGH (0)
#define BOUND_Y_LOW (0)
#define BOUND_Y_HIGH (0)

// Serialization operations

QDataStream& operator>>(QDataStream& stream, iez::PoseRecognition::Data &data)
{
	QString str;
	str.reserve(16);
	qint8 c;
	int index = 0;
	do {
		if (stream.atEnd() && index == 0) {
			stream.setStatus(QDataStream::ReadCorruptData);
			break;
		}
		stream >> c;
		if (c == ',') {
			if (str.isEmpty() || (index >= data.input.size())) {
				stream.setStatus(QDataStream::ReadCorruptData);
				break;
			}
			data.input[index++] = str.toFloat();
			str.clear();
		} else if (c != '\n' && c != '\r' && c != 0) {
			str += c;
		} else {
			if (str.isEmpty()) {
				stream.setStatus(QDataStream::ReadCorruptData);
				break;
			}
			data.output = str.toInt();
			str.clear();
			break;
		}

	} while (true);
	return stream;
}

QDataStream& operator>>(QDataStream& stream, QList<iez::PoseRecognition::Data> &data)
{
	iez::PoseRecognition::Data d(NN_INPUT_VECTOR_SIZE);

	do {
		stream >> d;
		data.append(d);
	} while (!stream.atEnd());

	stream.setStatus(QDataStream::Ok);
	return stream;
}


QDataStream& operator<<(QDataStream& stream, const iez::PoseRecognition::Data &data)
{
	for (int input = 0; input < NN_INPUT_VECTOR_SIZE; input++) {
		QByteArray str = QString::number(data.input[input], 'g', 10).toAscii();
		for (int i = 0; i < str.size(); i++) {
			stream << static_cast<qint8>(str[i]);
		}

		stream << static_cast<qint8>(',');
	}
	QByteArray str = QString::number(data.output, 'g', 10).toAscii();
	for (int i = 0; i < str.size(); i++) {
		stream << static_cast<qint8>(str[i]);
	}
	stream << static_cast<qint8>('\n');
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const QList<iez::PoseRecognition::Data> &data)
{
	foreach (iez::PoseRecognition::Data d, data) {
		stream << d;
	}

	return stream;
}

namespace iez {


PoseRecognition::PoseRecognition()
{
	m_neuralNetwork = 0;
}

void PoseRecognition::learnNew(const PoseRecognition::POSE pose,
							const cv::Point palmCenter,
							const float palmRadius,
							const wristpair_t &wrist,
							const QList<cv::Point> &fingertips)
{
	// cannot learn
	if (fingertips.size() < 1 || fingertips.size() > 5) {
		return;
	}

	OpenNN::Vector<double> featureVector = constructFeatureVector(palmCenter, palmRadius, wrist, fingertips);

	Q_ASSERT(featureVector.size() == NN_INPUT_VECTOR_SIZE);
	Data d(NN_INPUT_VECTOR_SIZE);
	int index = 0;
	foreach (double val, featureVector) {
		d.input[index++] = val;
	}
	d.output = pose;

	QMutexLocker l(&m_dbMutex);

	m_database.append(d);
	m_matrix = convertToNormalizedMatrix(m_database);
}

void PoseRecognition::savePoseDatabase(QString path)
{
	QMutexLocker l(&m_dbMutex);
	saveDatabaseToFile(path, m_database);
}

void PoseRecognition::loadPoseDatabase(QString path)
{
	QMutexLocker l(&m_dbMutex);

	m_database = loadDatabaseFromFile(path);
	if (m_database.size() != 0) {
		m_matrix = convertToNormalizedMatrix(m_database);
	} else {
		qDebug("Error loading pose database");
	}
}

void PoseRecognition::neuralNetworkSave(QString path)
{
	QMutexLocker l(&m_nnMutex);

	if (!m_neuralNetwork) {
		return;
	}
	m_neuralNetwork->save(path.toStdString());
}

void PoseRecognition::neuralNetworkLoad(QString path)
{
	QMutexLocker l(&m_nnMutex);

	if (!m_neuralNetwork) {
		m_neuralNetwork = new OpenNN::NeuralNetwork();
	}
	m_neuralNetwork->load(path.toStdString());
	testNeuralNetwork();
}

void PoseRecognition::neuralNetworkImport(QString path)
{
	QMutexLocker l(&m_nnMutex);

	OpenNN::MultilayerPerceptron mlp;
	if (loadMLP(path, mlp)) {
		if (m_neuralNetwork) {
			delete m_neuralNetwork;
		}
		m_neuralNetwork = new OpenNN::NeuralNetwork(mlp);
		testNeuralNetwork();
	} else {
		qDebug("error loading neural network");
	}
}

void PoseRecognition::train(int hiddenCount)
{
	using namespace OpenNN;

	QMutexLocker lockNN(&m_nnMutex);
	QMutexLocker lockDB(&m_dbMutex);

	if (m_matrix.get_rows_number() < 1) {
		return;
	}
	DataSet dataSet;

	dataSet.set(m_matrix);

	// Variables

	Variables* variablesPointer = dataSet.get_variables_pointer();

	variablesPointer->set_name(0, "f1x");
	variablesPointer->set_name(1, "f1y");
	variablesPointer->set_name(2, "f2x");
	variablesPointer->set_name(3, "f2y");
	variablesPointer->set_name(4, "f3x");
	variablesPointer->set_name(5, "f3y");
	variablesPointer->set_name(6, "f4x");
	variablesPointer->set_name(7, "f4y");
	variablesPointer->set_name(8, "f5x");
	variablesPointer->set_name(9, "f5y");

	for (int i = 0; i < NN_INPUT_VECTOR_SIZE; i++) {
		variablesPointer->set_use(i, Variables::Input);
	}
	for (int i = 0; i < POSE_END; i++) {
		variablesPointer->set_name(NN_INPUT_VECTOR_SIZE + i, QString("output_").append(QString::number(i)).toStdString());
		variablesPointer->set_use(NN_INPUT_VECTOR_SIZE + i, Variables::Target);
	}

	double errorRatio = NAN;
	do {
		// Instances

		Instances* instancesPointer = dataSet.get_instances_pointer();
		instancesPointer->split_random_indices(2.0, 0.2, 0.5);

		const Matrix<std::string> inputs_information = variablesPointer->arrange_inputs_information();
		const Matrix<std::string> targets_information = variablesPointer->arrange_targets_information();

//		// Neural network

		if (m_neuralNetwork) {
			delete m_neuralNetwork;
		}
		m_neuralNetwork = new NeuralNetwork(NN_INPUT_VECTOR_SIZE, hiddenCount, POSE_END);
		m_neuralNetwork->randomize_parameters_normal(-0.01, 0.01);

		Inputs* inputs_pointer = m_neuralNetwork->get_inputs_pointer();

		inputs_pointer->set_information(inputs_information);

		m_neuralNetwork->construct_scaling_layer();

		ScalingLayer* scaling_layer_pointer = m_neuralNetwork->get_scaling_layer_pointer();

		scaling_layer_pointer->set_scaling_method(ScalingLayer::NoScaling);

		MultilayerPerceptron* multilayer_perceptron_pointer = m_neuralNetwork->get_multilayer_perceptron_pointer();

		multilayer_perceptron_pointer->set_layer_activation_function(1, Perceptron::HyperbolicTangent);

		Outputs* outputs_pointer = m_neuralNetwork->get_outputs_pointer();

		outputs_pointer->set_information(targets_information);

		// Performance functional

		PerformanceFunctional performance_functional(m_neuralNetwork, &dataSet);

		// Training strategy

		TrainingStrategy training_strategy(&performance_functional);
		int method = 3;
		switch (method) {
		case 0:
			training_strategy.set_main_type(OpenNN::TrainingStrategy::CONJUGATE_GRADIENT);
			training_strategy.get_conjugate_gradient_pointer()->set_minimum_performance_increase(1.0e-6);
			break;

		case 1:
			training_strategy.set_main_type(OpenNN::TrainingStrategy::LEVENBERG_MARQUARDT_ALGORITHM);
			training_strategy.get_Levenberg_Marquardt_algorithm_pointer()->set_minimum_performance_increase(1.0e-6);
			break;

		default:
			training_strategy.set_main_type(OpenNN::TrainingStrategy::QUASI_NEWTON_METHOD);
			training_strategy.get_quasi_Newton_method_pointer()->set_minimum_performance_increase(1.0e-6);
			break;
		}

		TrainingStrategy::Results training_strategy_results = training_strategy.perform_training();
		QStringList params;
		foreach (double p, m_neuralNetwork->arrange_parameters()) {
			params.append(QString::number(p));
		}
		qDebug() << params;


		TestingAnalysis testing_analysis(m_neuralNetwork, &dataSet);
		Matrix<size_t> confusion = testing_analysis.calculate_confusion();
		confusion.save_csv("/tmp/confusion.csv");
//		qDebug("Generalization: %f", training_strategy_results.quasi_Newton_method_results_pointer->final_generalization_performance);
//		std::string resultString = "/tmp/ooo.txt";
//		training_strategy_results.save(resultString);
//		qDebug("%s\n\n", resultString.c_str());
		int errors = 0;
		int total = 0;
		for (int i = 0; i < confusion.get_rows_number(); i++) {
			Vector<size_t> row = confusion.arrange_row(i);
			for (int j = 0; j < row.size(); j++) {
				total += row[j];
				if (i != j) {
					errors += row[j];
					continue;
				}

			}
		}
		QFile f("/tmp/analysis");
		f.open(QFile::WriteOnly);
		QTextStream s(&f);
		s << errors << "/" << total;

		errorRatio = static_cast<double>(errors) / total;
		if (testNeuralNetwork()) break;
		::sleep(1);


	} while (true);
}

QString PoseRecognition::databaseToString() const
{
	QMutexLocker l(&m_dbMutex);

	QString output;
	foreach (Data d, m_database) {
		QString tmp;
		foreach (float f, d.input) {
			tmp.append(QString("%1 ").arg(f));
		}
		tmp.append(QString("||%1\n").arg(d.output));
		output.append(tmp);
	}
	return output;
}

QString PoseRecognition::categorize(const cv::Point palmCenter,
												  const float palmRadius,
												  const wristpair_t &wrist,
												  const QList<cv::Point> &fingertips) const
{
	if (!m_neuralNetwork) {
		return QString("Nope, just chuck testa");
	}

	const OpenNN::Vector<double> &featureVector = constructFeatureVector(palmCenter, palmRadius, wrist, fingertips);
	OpenNN::Vector<double> featureVectorNormalized(featureVector.size());

	qCopy(featureVector.begin(), featureVector.end(), featureVectorNormalized.begin());
	normalizeVector(featureVectorNormalized);

	const OpenNN::Vector<double> &outputs = m_neuralNetwork->get_multilayer_perceptron_pointer()->calculate_outputs(featureVectorNormalized);
	QVector<double> outputQVector = QVector<double>::fromStdVector(outputs);

	int minIndex = outputs.calculate_maximal_index();
//	double minValue = std::numeric_limits<double>::max();

//	for (int i = 0; i < outputQVector.size(); i++) {
//		double val = outputQVector[i];
//		double err = fabs(1 - val);
//		if (minValue > err) {
//			minValue = err;
//			minIndex = i;
//		}
//	}

	QString outputString;
	outputString += poseToString(minIndex) + "\n";

	foreach (double d, featureVectorNormalized) {
		outputString += QString::number(d, 'f', 3) + " ";
	}

	outputString += "\n\n##########\n";
	foreach (double val, outputQVector) {
		outputString += QString("%1\n").arg(val);
	}

//	}
//	emit foundPose(outputString);
//	qDebug("%s", outputString.toStdString().c_str());
	return outputString;
}

int PoseRecognition::calculateOutput(OpenNN::Vector<double> featureVector) const
{
	const OpenNN::Vector<double> &outputs = m_neuralNetwork->calculate_outputs(featureVector);

	return findBestMatchIndex(outputs, 1);
}

// Not thread safe
bool PoseRecognition::testNeuralNetwork() const
{
	int errors = 0;
	OpenNN::Vector<size_t> selectedColumns(NN_INPUT_VECTOR_SIZE);
	for (int i = 0; i < selectedColumns.size(); i++) {
		selectedColumns[i] = i;
	}

	for (int i = 0; i < m_database.size(); i++) {
		OpenNN::Vector<double> row(NN_INPUT_VECTOR_SIZE);

		qCopy(m_database.at(i).input.begin(), m_database.at(i).input.end(), row.begin());
		normalizeVector(row);

		int output = calculateOutput(row);

		if (output != m_database.at(i).output) {
			errors++;
		}
	}
	float ratio = static_cast<float>(errors)/m_database.size();
	qWarning("RATIO: %f", ratio);
	return ratio < 0.1f;
}

QString PoseRecognition::poseToString(enum POSE pose)
{
	switch (pose) {
		default:
			return QString::number(static_cast<int>(pose));
			break;
	}
}

QString PoseRecognition::poseToString(int pose)
{
	if (pose < POSE_END) {
		return poseToString(static_cast<enum POSE>(pose));
	} else {
		return "";
	}
}

QList<PoseRecognition::Data> PoseRecognition::loadDatabaseFromFile(QString path)
{
	QList<PoseRecognition::Data> data;
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		return data;
	}
	QDataStream s(&f);
	s >> data;
	if (s.status() == QDataStream::Ok) {
		return data;
	} else {
		return QList<PoseRecognition::Data>();
	}
}

void PoseRecognition::saveDatabaseToFile(QString path, QList<PoseRecognition::Data> database)
{
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly)) {
		return;
	}
	QDataStream s(&f);
	s << database;
}

int PoseRecognition::inputVectorSize()
{
	return NN_INPUT_VECTOR_SIZE;
}

double PoseRecognition::normalizeInto(double value, double low, double high)
{
	if (low == 0 && high == 0) {
		return value;
	}
	Q_ASSERT(low < high);
	double range = high - low;

	if (value < low) {
		return 0.0;
	} else if (value > high) {
		return 1.0;
	} else {
		return (value - low)/range;
	}
}


/**
 * Assumes Logistic layers
 */
bool PoseRecognition::loadLayer(QFile &fBias, QFile &fWeights, const int inputCount, const int neuronCount, OpenNN::PerceptronLayer &layer)
{
	bool ret = true;
	if (!fBias.open(QFile::ReadOnly)) {
		ret = false;
	}
	if (!fWeights.open(QFile::ReadOnly)) {
		ret = false;
	}
	Q_ASSERT(ret);
	QTextStream wStream(&fWeights);
	QTextStream bStream(&fBias);
	int index;
	// input weights
	OpenNN::Matrix<double> weightsData(neuronCount, inputCount);
	index = 0;
	while (!wStream.atEnd()) {
		Q_ASSERT(index < neuronCount);
		QString line = wStream.readLine();
		QStringList lineSplitted = line.split(",");
		Q_ASSERT(lineSplitted.size() == inputCount);
		OpenNN::Vector<double> row(lineSplitted.size());
		for (int i = 0; i < row.size(); i++) {
			bool ok = false;
			row[i] = lineSplitted[i].toDouble(&ok);
			Q_ASSERT(ok);
		}
		weightsData.set_row(index++, row);
	}
	Q_ASSERT(index == neuronCount);

	// input biases
	OpenNN::Vector<double> biasData(neuronCount);
	index = 0;
	while (!bStream.atEnd()) {
		Q_ASSERT(index < neuronCount);
		QString line = bStream.readLine();
		bool ok = false;
		biasData[index++] = line.toDouble(&ok);
		Q_ASSERT(ok);
	}
	Q_ASSERT(index == neuronCount);

	OpenNN::PerceptronLayer inputLayer(inputCount, neuronCount);
	inputLayer.set_activation_function(OpenNN::Perceptron::Logistic);
	inputLayer.set_biases(biasData);
	inputLayer.set_synaptic_weights(weightsData);

	layer = inputLayer;
	return true;
}

bool PoseRecognition::loadMLP(QString prefix, OpenNN::MultilayerPerceptron &mlp)
{
	QFile metaFile(prefix + "/metadata.csv");

	if (!metaFile.open(QFile::ReadOnly)) {
		qDebug("cannot open metadata");
		return false;
	}

	QTextStream metaStream(&metaFile);
	QString metaLine = metaStream.readLine();
	QStringList metaLineList = metaLine.split(",");

	if (metaLineList.size() != 3) {
		qDebug("invalid number of metadata data");
		return false;
	}

	bool ok = true;
	const int inputCount = metaLineList[0].toInt(&ok);
	if (inputCount != NN_INPUT_VECTOR_SIZE || !ok) {
		qDebug("invalid size of input vector");
		return false;
	}
	const int hiddenCount = metaLineList[1].toInt(&ok);
	if (!ok) {
		qDebug("cannot read number of hidden neurons");
		return false;
	}
	const int outputCount = metaLineList[2].toInt(&ok);
	if (!ok) {
		qDebug("cannot read number of output neurons");
		return false;
	}

	QFile iwFile(prefix + "/neural_data_INPUT_W.csv");
	QFile ibFile(prefix + "/neural_data_INPUT_B.csv");
	QFile hwFile(prefix + "/neural_data_HIDDEN_W.csv");
	QFile hbFile(prefix + "/neural_data_HIDDEN_B.csv");

	OpenNN::Vector<OpenNN::PerceptronLayer> layers(2);

	if (!loadLayer(ibFile, iwFile, inputCount, hiddenCount, layers[0])) return false;
	if (!loadLayer(hbFile, hwFile, hiddenCount, outputCount, layers[1])) return false;

	mlp.set_layers(layers);
	qDebug("success");
	return true;
}

void PoseRecognition::normalizeVector(OpenNN::Vector<double> &vec)
{
	for (int i = 0; i < NN_INPUT_VECTOR_SIZE; i += 2) {
		vec[i] = normalizeInto(vec[i], BOUND_X_LOW, BOUND_X_HIGH);//x
		vec[i + 1] = normalizeInto(vec[i + 1], BOUND_Y_LOW, BOUND_Y_HIGH);//y
	}
}

OpenNN::Matrix<double> PoseRecognition::convertToNormalizedMatrix(const QList<Data> &db)
{
	// normalize X from <-3, 3> -> <0, 1>
	OpenNN::Matrix<double> mat(db.size(), NN_INPUT_VECTOR_SIZE + POSE_END);
	// convert To Matrix
	int index = 0;
	foreach (Data data, db) {
		OpenNN::Vector<double> k(NN_INPUT_VECTOR_SIZE + POSE_END, 0);
		Q_ASSERT(data.input.size() == NN_INPUT_VECTOR_SIZE);
		qCopy(data.input.begin(), data.input.end(), k.begin());
		normalizeVector(k);

		Q_ASSERT(data.output >= 0 && data.output < POSE_END);
		k[NN_INPUT_VECTOR_SIZE + data.output] = 1;

//		QStringList rowString;
//		foreach (double d, k) rowString.append(QString::number(d));
//		qDebug() << "____________________________"<< rowString;

		mat.set_row(index++, k);
	}
	return mat;

}

/**
 *  assume fingertips are ordered
 */
OpenNN::Vector<double> PoseRecognition::constructFeatureVector( const cv::Point palmCenter,
																const float palmRadius,
																const wristpair_t &wrist,
																const QList<cv::Point> &fingertips)
{
	const float norm = palmRadius;
	// normalize
	cv::Point2f b1_tmp = (wrist.second - wrist.first);
	b1_tmp.x /= norm;
	b1_tmp.y /= norm;
	const cv::Point2f b1 = b1_tmp;
	const cv::Point2f b2 = cv::Point2f(b1.y, -b1.x);
	const cv::Point2f wristMiddlePoint = Util::pointMean(wrist.first, wrist.second);

	OpenNN::Vector<double> featureVector(NN_INPUT_VECTOR_SIZE, 0);
	if (fingertips.size() > 5) {
		return featureVector;
	}

	int i = 0;
	foreach (const cv::Point2f p, fingertips) {
		const cv::Point2f v = p - wristMiddlePoint;
		const cv::Point2f vNorm = cv::Point2f(v.x/norm, v.y/norm);
		const cv::Point2f vNormRot = cv::Point2f(b1.dot(vNorm), b2.dot(vNorm));
		featureVector.at(i++) = vNormRot.x;
		featureVector.at(i++) = vNormRot.y;
	}

	return featureVector;
}


}
