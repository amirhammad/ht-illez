#include "PoseRecognition.h"
#include "Processing.h"

#include <opennn.h>

#include <QVector>


#define INPUT_VECTOR_SIZE 11
#define NNDATA_FILENAME "NNData.txt"
// define stream operators

QDataStream& operator<<(QDataStream& stream, const OpenNN::Perceptron& foo)
{
	stream << static_cast<unsigned int>(foo.get_inputs_number());
	stream << foo.get_bias();
	stream << static_cast<unsigned int>(foo.get_activation_function());
	stream << foo.get_display();
	for (int i = 0; i < foo.get_inputs_number(); i++) {
		stream << foo.get_synaptic_weight(i);
	}

	return stream;
}

QDataStream& operator>>(QDataStream& stream, OpenNN::Perceptron& foo)
{
	int inputs_number = 0;
	stream >> inputs_number;
	foo.set_inputs_number(5);

	double bias = 0;
	stream >> bias;
	foo.set_bias(bias);

	unsigned int actFunction;
	stream >> actFunction;
	foo.set_activation_function(static_cast<OpenNN::Perceptron::ActivationFunction>(actFunction));

	bool display = false;
	stream >> display;
	foo.set_display(display);

	for (int i = 0; i < foo.get_inputs_number(); i++) {
		double tmp;
		stream >> tmp;
		foo.set_synaptic_weight(i, tmp);
	}

	return stream;
}


QDataStream& operator<<(QDataStream& stream, const OpenNN::PerceptronLayer& foo)
{
	stream << static_cast<unsigned int>(foo.get_inputs_number());
	stream << static_cast<unsigned int>(foo.get_activation_function());
	stream << foo.get_display();
	stream << static_cast<unsigned int>(foo.get_perceptrons_number());
	for (int i = 0; i < foo.get_perceptrons_number(); i++) {
		stream << foo.get_perceptron(i);
	}

	return stream;
}

QDataStream& operator>>(QDataStream& stream, OpenNN::PerceptronLayer& foo)
{
	int inputs_number = 0;
	stream >> inputs_number;
	foo.set_inputs_number(5);


	unsigned int actFunction;
	stream >> actFunction;
	foo.set_activation_function(static_cast<OpenNN::Perceptron::ActivationFunction>(actFunction));

	bool display = false;
	stream >> display;
	foo.set_display(display);

	unsigned int perceptron_number = 0;
	stream >> perceptron_number;
	foo.set_perceptrons_number(perceptron_number);

	for (int i = 0; i < perceptron_number; i++) {
		OpenNN::Perceptron tmp;
		stream >> tmp;
		foo.set_perceptron(i, tmp);
	}

	return stream;
}

//QDataStream& operator<<(QDataStream& stream, const OpenNN::Matrix<double>& foo)
//{
//	for (int i = 0; i < foo.rows_number; i++) {
//		for (int j = 0; j < foo.columns_number; j++) {
//			stream << foo.arrange_column()
//		}
//	}

//	return stream;
//}

//QDataStream& operator>>(QDataStream& stream, OpenNN::Matrix<double>& foo)
//{


//	return stream;
//}



namespace iez {


PoseRecognition::PoseRecognition()
{
	qRegisterMetaTypeStreamOperators<OpenNN::Perceptron>("OpenNN::Perceptron");
	qRegisterMetaTypeStreamOperators<OpenNN::PerceptronLayer>("OpenNN::PerceptronLayer");
	qRegisterMetaType<OpenNN::Perceptron>();
	qRegisterMetaType<OpenNN::PerceptronLayer>();


	m_settings = new QSettings("pose_recognition.ini", QSettings::IniFormat);
	QVariant varSettings = m_settings->value("NN");
	if (varSettings.canConvert<OpenNN::Perceptron>()) {
		qDebug("asdasd");
		const OpenNN::Perceptron &perc = varSettings.value<OpenNN::Perceptron>();
		qDebug("Bias=%f %f", perc.get_bias(), perc.get_synaptic_weight(0));


		OpenNN::Vector<OpenNN::Perceptron> pl(1);
		pl[0] = perc;

		OpenNN::PerceptronLayer percLayer;
		percLayer.set_perceptrons(pl);
		OpenNN::MultilayerPerceptron mlp;
		mlp.get_layers();
		OpenNN::ConjugateGradient conjGradient;
		conjGradient.get_display();
	}
	try {
		m_matrix.load(NNDATA_FILENAME);
		if (m_matrix.empty()) {
			m_matrix.set_columns_number(INPUT_VECTOR_SIZE + POSE_END);
		}
	} catch (...) {
		m_matrix = OpenNN::Matrix<double> ();
//		m_matrix.set_columns_number(INPUT_VECTOR_SIZE + POSE_END);
//		m_matrix.save(NNDATA_FILENAME);
	}



	OpenNN::Perceptron perceptron(5);
	perceptron.set_activation_function("Linear");
	perceptron.set_synaptic_weight(0, 1);
	perceptron.set_synaptic_weight(1, 0);
	perceptron.set_synaptic_weight(2, 0);
	perceptron.set_synaptic_weight(3, 0);
	perceptron.set_synaptic_weight(4, 0);
	perceptron.set_bias(5);
	OpenNN::Vector<double> vec(5);
	vec[0] = 2;
	vec[1] = 3;
	vec[2] = 4;
	vec[3] = 5;
	vec[4] = 6;
	printf("VALUE: %f\n", perceptron.calculate_output(vec));
	m_settings->setValue("NN", qVariantFromValue(perceptron));


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

	OpenNN::Vector<double> featureVector = constructFeatureVector(palmCenter, palmRadius, wrist, fingertips, pose);
	appendToMatrix(featureVector);
//	learn();
}

void PoseRecognition::learn()
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
	for (int i = 0; i < POSE_END; i++) {
		variablesPointer->set_name(INPUT_VECTOR_SIZE + i, QString("output_").append(QString::number(i)).toStdString());
	}

	// Instances

	Instances* instancesPointer = dataSet.get_instances_pointer();

	instancesPointer->split_random_indices(0.75, 0.0, 0.25);

	const Matrix<std::string> inputs_information = variablesPointer->arrange_inputs_information();
	const Matrix<std::string> targets_information = variablesPointer->arrange_targets_information();

	const Vector< Statistics<double> > inputs_statistics = dataSet.scale_inputs_minimum_maximum();

	// Neural network
	Vector<size_t> nnSizes(4);
	nnSizes[0] = INPUT_VECTOR_SIZE;
	nnSizes[1] = 23;
	nnSizes[2] = 33;
	nnSizes[3] = POSE_END;
	NeuralNetwork neural_network(nnSizes);

	Inputs* inputs_pointer = neural_network.get_inputs_pointer();

	inputs_pointer->set_information(inputs_information);

	neural_network.construct_scaling_layer();

	ScalingLayer* scaling_layer_pointer = neural_network.get_scaling_layer_pointer();

	scaling_layer_pointer->set_statistics(inputs_statistics);

	scaling_layer_pointer->set_scaling_method(ScalingLayer::NoScaling);

	MultilayerPerceptron* multilayer_perceptron_pointer = neural_network.get_multilayer_perceptron_pointer();

	multilayer_perceptron_pointer->set_layer_activation_function(1, Perceptron::Logistic);
	multilayer_perceptron_pointer->set_layer_activation_function(2, Perceptron::HyperbolicTangent);

	Outputs* outputs_pointer = neural_network.get_outputs_pointer();

	outputs_pointer->set_information(targets_information);

	// Performance functional

	PerformanceFunctional performance_functional(&neural_network, &dataSet);

	// Training strategy

	TrainingStrategy training_strategy(&performance_functional);

	QuasiNewtonMethod* quasi_Newton_method_pointer = training_strategy.get_quasi_Newton_method_pointer();

	quasi_Newton_method_pointer->set_minimum_performance_increase(1.0e-6);

	TrainingStrategy::Results training_strategy_results = training_strategy.perform_training();

	std::string resultString;
	training_strategy_results.save(resultString);
}

PoseRecognition::POSE PoseRecognition::categorize(const wristpair_t &wrist,
												  const QList<cv::Point> &fingertips)
{
	return POSE_0;
}

OpenNN::Vector<double> PoseRecognition::constructFeatureVector( const cv::Point palmCenter,
																const float palmRadius,
																const wristpair_t &wrist,
																const QList<cv::Point> &fingertips,
																const PoseRecognition::POSE pose)
{
	const float norm = palmRadius;
	const cv::Point b1 = wrist.second - wrist.first;
	const cv::Point b2 = cv::Point(b1.y, -b1.x);
	const cv::Point2f wristMiddlePoint = Processing::pointMean(wrist.first, wrist.second);

	OpenNN::Vector<double> featureVector(INPUT_VECTOR_SIZE + static_cast<int>(PoseRecognition::POSE_END), 0);
	featureVector[10] = fingertips.size();

	QList<cv::Point> fingertipsOrdered = fingertips;
	// sort by angle
	struct compare {
		compare(cv::Point palmCenter, float offs)
		:	m_palmCenter(palmCenter)
		,	m_offs(offs) {

		}

		float getAngle(cv::Point point) {
			const float dy = point.y - m_palmCenter.y;
			const float dx = point.x - m_palmCenter.x;
			const float angle = (atan2f(dy, dx)) - (m_offs);
			if (angle < -M_PI) {
				return angle + 2*M_PI;
			} else {
				return angle;
			}
		}

		bool operator() (cv::Point a, cv::Point b) {
			return getAngle(a) < getAngle(b);
		}

		const cv::Point m_palmCenter;
		const float m_offs;
	};
//TODO: order not working properly? test more
	// ordering
	const float offs = atan2f(wrist.second.y - wrist.first.y,
						wrist.second.x - wrist.first.x);

	std::sort(fingertipsOrdered.begin(), fingertipsOrdered.end(), compare(cv::Point(wristMiddlePoint.x, wristMiddlePoint.y), offs));




	// [0] must be the first finger in direction of thumb->pointer
	int i = 0;
	foreach (const cv::Point2f p, fingertipsOrdered) {
		const cv::Point2f v = p - wristMiddlePoint;
		const cv::Point2f vNorm = cv::Point2f(v.x/norm, v.y/norm);
		const cv::Point2f vNormRot = cv::Point2f(b1.dot(vNorm), b2.dot(vNorm));
		featureVector[i++] = vNormRot.x;
		featureVector[i++] = vNormRot.y;
	}


	featureVector[INPUT_VECTOR_SIZE + POSE_5] = 3.0;

	return featureVector;
}

void PoseRecognition::appendToMatrix(OpenNN::Vector<double> vec)
{
	printf("%d %d\n", m_matrix.get_columns_number(), vec.size());
	if (m_matrix.empty()) {
		m_matrix.set(1, INPUT_VECTOR_SIZE + PoseRecognition::POSE_END);
	}
	m_matrix.append_row(vec);
	m_matrix.save(NNDATA_FILENAME);
}

}
Q_DECLARE_METATYPE(OpenNN::Perceptron)
Q_DECLARE_METATYPE(OpenNN::PerceptronLayer)



