#include "PoseRecognition.h"
#include "Processing.h"

#include <opennn.h>

#include <QVector>
#include <QVariantList>

#define INPUT_VECTOR_SIZE 11
#define NNDATA_FILENAME "NNData.txt"
namespace iez {


PoseRecognition::PoseRecognition()
{
	m_neuralNetwork = 0;
//	qRegisterMetaTypeStreamOperators<OpenNN::Perceptron>("OpenNN::Perceptron");
//	qRegisterMetaTypeStreamOperators<OpenNN::PerceptronLayer>("OpenNN::PerceptronLayer");
//	qRegisterMetaType<OpenNN::Perceptron>();
//	qRegisterMetaType<OpenNN::PerceptronLayer>();

	m_settings = new QSettings("pose_recognition.ini", QSettings::IniFormat);


	QVariant settingsVariant = m_settings->value("NNDATA");
	if (settingsVariant.isNull()) {
		qDebug("Error loading settings");
		return;
	}

	Q_ASSERT(settingsVariant.canConvert<QVariantList>());
	QVariantList list = settingsVariant.value<QVariantList>();

	foreach (QVariant v, list) {
		Q_ASSERT(v.canConvert<QVariantList>());
		QVariantList unitVariant = v.value<QVariantList>();

		Data d;
		d.input = QVector<float>(INPUT_VECTOR_SIZE);

		// save input
		for (int i = 0; i < INPUT_VECTOR_SIZE; i++) {
			Q_ASSERT(unitVariant.at(i).canConvert<float>());
			d.input[i] = unitVariant.at(i).toFloat();
		}
		// save output
		Q_ASSERT(unitVariant.at(INPUT_VECTOR_SIZE).canConvert<int>());
		d.output = unitVariant.at(INPUT_VECTOR_SIZE).toInt();

		m_database.append(d);
	}
	m_matrix = convertToMatrix(m_database);

	return;

//	OpenNN::Perceptron perceptron(5);
//	perceptron.set_activation_function("Linear");
//	perceptron.set_synaptic_weight(0, 1);
//	perceptron.set_synaptic_weight(1, 0);
//	perceptron.set_synaptic_weight(2, 0);
//	perceptron.set_synaptic_weight(3, 0);
//	perceptron.set_synaptic_weight(4, 0);
//	perceptron.set_bias(5);
//	OpenNN::Vector<double> vec(5);
//	vec[0] = 2;
//	vec[1] = 3;
//	vec[2] = 4;
//	vec[3] = 5;
//	vec[4] = 6;
//	printf("VALUE: %f\n", perceptron.calculate_output(vec));
//	m_settings->setValue("NN", qVariantFromValue(perceptron));


//	fflush(stdout);
//	exit(0);
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

	Data d;
	d.input = QVector<float>(featureVector.size(), 0);
	int index = 0;
	foreach (double val, featureVector) {
		d.input[index++] = val;
	}
	d.output = pose;
	m_database.append(d);
	m_matrix = convertToMatrix(m_database);
}

void PoseRecognition::savePoseDatabase()
{
	QVariantList output;
	foreach (Data d, m_database) {
		QVariantList v;
		foreach (float x, d.input) {
			v.append(x);
		}
		v.append(d.output);
		output.append(QVariant(v));
	}
	m_settings->setValue("NNDATA", output);
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
	variablesPointer->set_name(10, "count");

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
		tmp.append(QString(" || %1\r\n").arg(d.output));
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

	const OpenNN::Vector<double> &outputs = m_neuralNetwork->get_multilayer_perceptron_pointer()->calculate_outputs(featureVector);
	QVector<double> outputQVector = QVector<double>::fromStdVector(outputs);

	int minIndex = 0;
	double minValue = std::numeric_limits<double>::max();

	for (int i = 0; i < outputQVector.size(); i++) {
		double val = outputQVector[i];
		double err = fabs(1 - val);
		if (minValue > err) {
			minValue = err;
			minIndex = i;
		}
	}
	QString outputString;
	outputString += poseToString(minIndex);
	if (minValue > 0.5f) {
		outputString += QString("   Not Found");
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

OpenNN::Matrix<double> PoseRecognition::convertToMatrix(const QList<PoseRecognition::Data>& db)
{

	OpenNN::Matrix<double> mat(db.size(), INPUT_VECTOR_SIZE + POSE_END);
	// convert To Matrix
	int index = 0;
	foreach (Data data, db) {
		OpenNN::Vector<double> k(INPUT_VECTOR_SIZE + POSE_END, 0);
		Q_ASSERT(data.input.size() == INPUT_VECTOR_SIZE);
		for (int i = 0; i < INPUT_VECTOR_SIZE; i++) {
			k[i] = data.input[i];
		}
		Q_ASSERT(data.output >= 0 && data.output < POSE_END);
		k[INPUT_VECTOR_SIZE + data.output] = 1;

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
	featureVector[10] = fingertips.size();

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
Q_DECLARE_METATYPE(OpenNN::Perceptron)
Q_DECLARE_METATYPE(OpenNN::PerceptronLayer)



