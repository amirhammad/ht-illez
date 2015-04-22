#include "PoseRecognition.h"
#include "Processing.h"

#include <opennn.h>

#include <QVector>
#include <QFile>
#include <QVariantList>

#define INPUT_VECTOR_SIZE 11
#define NNDATA_FILENAME "NNData.csv"

// Serialization operations
#define BOUND_X_LOW (-3.0)
#define BOUND_X_HIGH (3.0)
#define BOUND_Y_LOW (-6.0)
#define BOUND_Y_HIGH (0.0)

QDataStream& operator>>(QDataStream& stream, iez::PoseRecognition::Data &data)
{
	QString str;
	str.reserve(16);
	qint8 c;
	int index = 0;
	do {
		if (stream.atEnd() && index == 0) {
			stream.setStatus(QDataStream::ReadCorruptData);
			return stream;
		}
		stream >> c;
		if (c == ',') {
			if (str.isEmpty()) {
				stream.setStatus(QDataStream::ReadCorruptData);
				return stream;
			}
			data.input[index++] = str.toFloat();
			str.clear();
		} else if (c != '\n' && c != '\r' && c != 0) {
			str += c;
		} else {
			if (str.isEmpty()) {
				stream.setStatus(QDataStream::ReadCorruptData);
				return stream;
			}
			data.output = str.toInt();
			str.clear();
			break;
		}

	} while (true);
}

QDataStream& operator>>(QDataStream& stream, QList<iez::PoseRecognition::Data> &data)
{
	iez::PoseRecognition::Data d(INPUT_VECTOR_SIZE);

	do {
		stream >> d;
		data.append(d);
	} while (!stream.atEnd());

	stream.setStatus(QDataStream::Ok);
	return stream;
}


QDataStream& operator<<(QDataStream& stream, const iez::PoseRecognition::Data &data)
{
	for (int input = 0; input < INPUT_VECTOR_SIZE; input++) {
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
	m_database = loadDatabaseFromFile(NNDATA_FILENAME);
	if (m_database.size() > 0) {
		m_matrix = convertToNormalizedMatrix(m_database);
	}
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

	Q_ASSERT(featureVector.size() == INPUT_VECTOR_SIZE);
	Data d(INPUT_VECTOR_SIZE);
	int index = 0;
	foreach (double val, featureVector) {
		d.input[index++] = val;
	}
	d.output = pose;
	m_database.append(d);
	m_matrix = convertToNormalizedMatrix(m_database);
}

void PoseRecognition::savePoseDatabase()
{
	saveDatabaseToFile(NNDATA_FILENAME, m_database);
}

void PoseRecognition::neuralNetworkSave(std::string path)
{
	if (!m_neuralNetwork) {
		return;
	}
	m_neuralNetwork->save(path);
}

void PoseRecognition::neuralNetworkLoad(std::string path)
{
	if (!m_neuralNetwork) {
		m_neuralNetwork = new OpenNN::NeuralNetwork();
	}
	m_neuralNetwork->load(path);
}

void PoseRecognition::train()
{
	using namespace OpenNN;

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

	for (int i = 0; i < INPUT_VECTOR_SIZE; i++) {
		variablesPointer->set_use(i, Variables::Input);
	}
	for (int i = 0; i < POSE_END; i++) {
		variablesPointer->set_name(INPUT_VECTOR_SIZE + i, QString("output_").append(QString::number(i)).toStdString());
		variablesPointer->set_use(INPUT_VECTOR_SIZE + i, Variables::Target);
	}

	do {
		// Instances

		Instances* instancesPointer = dataSet.get_instances_pointer();

		instancesPointer->split_random_indices(0.5, 0.25, 0.25);

		const Matrix<std::string> inputs_information = variablesPointer->arrange_inputs_information();
		const Matrix<std::string> targets_information = variablesPointer->arrange_targets_information();


		const Vector< Statistics<double> > inputs_statistics = dataSet.scale_inputs_minimum_maximum();


		// Neural network
		Vector<size_t> nnSizes(4);
		nnSizes[0] = INPUT_VECTOR_SIZE;
		nnSizes[1] = 35;
		nnSizes[2] = 25;
		nnSizes[3] = POSE_END;
		if (m_neuralNetwork) {
			delete m_neuralNetwork;
		}
		m_neuralNetwork = new NeuralNetwork(nnSizes);

		Inputs* inputs_pointer = m_neuralNetwork->get_inputs_pointer();

		inputs_pointer->set_information(inputs_information);

		m_neuralNetwork->construct_scaling_layer();

		ScalingLayer* scaling_layer_pointer = m_neuralNetwork->get_scaling_layer_pointer();

		scaling_layer_pointer->set_statistics(inputs_statistics);

		scaling_layer_pointer->set_scaling_method(ScalingLayer::NoScaling);

		MultilayerPerceptron* multilayer_perceptron_pointer = m_neuralNetwork->get_multilayer_perceptron_pointer();

		multilayer_perceptron_pointer->set_layer_activation_function(1, Perceptron::HyperbolicTangent);
		multilayer_perceptron_pointer->set_layer_activation_function(2, Perceptron::Logistic);

		Outputs* outputs_pointer = m_neuralNetwork->get_outputs_pointer();

		outputs_pointer->set_information(targets_information);

		// Performance functional

		PerformanceFunctional performance_functional(m_neuralNetwork, &dataSet);

		// Training strategy

		TrainingStrategy training_strategy(&performance_functional);

		QuasiNewtonMethod* quasi_Newton_method_pointer = training_strategy.get_quasi_Newton_method_pointer();

		quasi_Newton_method_pointer->set_minimum_performance_increase(1.0e-6);

		TrainingStrategy::Results training_strategy_results = training_strategy.perform_training();

		qDebug("Generalization: %f", training_strategy_results.quasi_Newton_method_results_pointer->final_generalization_performance);
		std::string resultString;
		training_strategy_results.save(resultString);
		qDebug("%s", resultString.c_str());

	} while (!testNeuralNetwork());
}

QString PoseRecognition::databaseToString() const
{
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
												  const QList<cv::Point> &fingertips)
{
	if (!m_neuralNetwork) {
		return QString("Nope, just chuck testa");
	}

	const OpenNN::Vector<double> &featureVector = constructFeatureVector(palmCenter, palmRadius, wrist, fingertips);
	OpenNN::Vector<double> featureVectorNormalized(featureVector.size());

	qCopy(featureVector.begin(), featureVector.end(), featureVectorNormalized.begin());
	normalizeVector(featureVectorNormalized);

	const OpenNN::Vector<double> &outputs = m_neuralNetwork->get_multilayer_perceptron_pointer()->calculate_outputs(featureVector);
	QVector<double> outputQVector = QVector<double>::fromStdVector(outputs);

	int minIndex = outputs.calculate_maximal_index();
	double minValue = std::numeric_limits<double>::max();

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
	const OpenNN::Vector<double> &outputs = m_neuralNetwork->get_multilayer_perceptron_pointer()->calculate_outputs(featureVector);

	int minIndex = 0;
	double minValue = std::numeric_limits<double>::max();

	for (int i = 0; i < outputs.size(); i++) {
		double val = outputs[i];
		double err = fabs(1 - val);
		if (minValue > err) {
			minValue = err;
			minIndex = i;
		}
	}

	return minIndex;
}

bool PoseRecognition::testNeuralNetwork() const
{
	int errors = 0;
	OpenNN::Vector<size_t> selectedColumns(INPUT_VECTOR_SIZE);
	for (int i = 0; i < selectedColumns.size(); i++) {
		selectedColumns[i] = i;
	}

	for (int i = 0; i < m_matrix.get_rows_number(); i++) {
		OpenNN::Vector<double> row = m_matrix.arrange_row(i, selectedColumns);
		int output = calculateOutput(row);
		if (output != m_database.at(i).output) {
			errors++;
		}
	}
	float ratio = static_cast<float>(errors)/m_matrix.get_rows_number();
	qWarning("RATIO: %f", ratio);
	return ratio < 0.4f;
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

	return data;
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
	return INPUT_VECTOR_SIZE;
}

double PoseRecognition::normalizeInto(double value, double low, double high)
{
	Q_ASSERT(low <= high);
	if (low == 0.0 && high == 0.0) {
		return value;
	}
	double range = high - low;

	if (value < low) {
		return 0.0;
	} else if (value > high) {
		return 1.0;
	} else {
		return (value - low)/range;
	}
}

bool PoseRecognition::loadLayer(QFile &fBias, QFile &fWeights, const int inputCount, const int neuronCount, OpenNN::PerceptronLayer &layer)
{
	if (!fBias.open(QFile::ReadOnly)) return false;
	if (!fWeights.open(QFile::ReadOnly)) return false;
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
	const int iwRowCount = 25;
	const int iwColCount = 10;
	const int ibRowCount = 25;
	const int hwRowCount = 8;
	const int hwColCount = 25;
	const int hbRowCount = 8;

	QFile iwFile(prefix + "/neural_data_INPUT_W.csv");
	QFile ibFile(prefix + "/neural_data_INPUT_B.csv");
	QFile hwFile(prefix + "/neural_data_HIDDEN_W.csv");
	QFile hbFile(prefix + "/neural_data_HIDDEN_B.csv");

	OpenNN::Vector<OpenNN::PerceptronLayer> layers(2);

	if (!loadLayer(ibFile, iwFile, iwColCount, iwRowCount, layers[0])) return false;
	if (!loadLayer(hbFile, hwFile, hwColCount, hwRowCount, layers[1])) return false;

	mlp.set_layers(layers);
	qDebug("success");
	return true;
}

void PoseRecognition::normalizeVector(OpenNN::Vector<double> &vec)
{
	for (int i = 0; i < NN_INPUT_VECTOR_SIZE; i += 2) {
		if (vec[i] == 0 && vec[i + 1] == 0) {
			vec[i] = 0;
			vec[i + 1] = 0;
		} else {
			vec[i] = normalizeInto(vec[i], BOUND_X_LOW, BOUND_X_HIGH);//x
			vec[i + 1] = normalizeInto(vec[i + 1], BOUND_Y_LOW, BOUND_Y_HIGH);//y
		}
	}
}

OpenNN::Matrix<double> PoseRecognition::convertToNormalizedMatrix(const QList<Data> &db)
{
	// normalize X from <-3, 3> -> <0, 1>
	OpenNN::Matrix<double> mat(db.size(), NN_INPUT_VECTOR_SIZE + POSE_END);
	// convert To Matrix
	int index = 0;
	foreach (Data data, db) {
		OpenNN::Vector<double> k(INPUT_VECTOR_SIZE + POSE_END, 0);
		Q_ASSERT(data.input.size() == INPUT_VECTOR_SIZE);
		qCopy(data.input.begin(), data.input.end(), k.begin());
		normalizeVector(k);

		Q_ASSERT(data.output >= 0 && data.output < POSE_END);
		k[INPUT_VECTOR_SIZE + data.output] = 1;

//		QStringList rowString;
//		foreach (double d, k) rowString.append(QString::number(d));
//		qDebug() << "____________________________"<< rowString;

		mat.set_row(index++, k);
	}
	return mat;
}


/*
 *  assume, fingertips are ordered
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
	const cv::Point2f wristMiddlePoint = Processing::pointMean(wrist.first, wrist.second);
//	qDebug("###(%f %f)", b2.x, b2.y);
	OpenNN::Vector<double> featureVector(INPUT_VECTOR_SIZE, 0);
	if (fingertips.size() > 5) {
		return featureVector;
	}

	// [0] must be the first finger in direction of thumb->pointer
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

void PoseRecognition::appendToMatrix(OpenNN::Vector<double> vec)
{
	printf("%lu %lu\n", m_matrix.get_columns_number(), vec.size());
	if (m_matrix.empty()) {
		m_matrix.set(1, INPUT_VECTOR_SIZE + PoseRecognition::POSE_END);
		m_matrix.set_row(1, vec);
	}
	m_matrix.append_row(vec);
	m_matrix.save(NNDATA_FILENAME);
}

}
