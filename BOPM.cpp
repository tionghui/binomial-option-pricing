#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <utility>

double binomialCoeff(int N, int r) {
	double result = 1;
	for (int i = 1; i <= r; i++) {
		result *= (N - r + i);
		result /= i;
	}
	return result;
}

class BOPM {
private:
	int steps;				// number of steps
	double S0;				// initial price
	double S1;				// next step price
	double probUp;			// probability of upward movement
	double freq;			// price change frequency per year
	double maturity;		// duration of option in years
	double change;			// price change %

	std::vector<std::vector<double>> priceTree;
	std::vector<std::vector<double>> probTree;

public:
	// constructor
	BOPM(double S0_, double S1_, double probUp_, double freq_, double maturity_)
		: steps(static_cast<int>(freq_* maturity_)),
		S0(S0_), S1(S1_), probUp(probUp_), freq(freq_), maturity(maturity_),
		change(abs(S1_ - S0_) / S0_) {
	};

	// binomial tree, return final prices & probabilities
	std::pair<std::vector<double>, std::vector<double>> buildTree();

	// export the binomial tree to CSV
	int exportCSV(const std::string& filename) const;

	// pricing
	double callPrice(const std::vector<double>& finalPrices,
					 const std::vector<double>& finalProbs,
					 double strike, double r, double freq) const;

	double putPrice(const std::vector<double>& finalPrices,
					const std::vector<double>& finalProbs,
					double strike, double r, double freq) const;

	// print results
	void printResults(const std::string& optionType,
					  const std::vector<double>& finalPrices,
					  const std::vector<double>& finalProbs,
					  double strike, double price) const;
};

std::pair<std::vector<double>, std::vector<double>> BOPM::buildTree() {
	priceTree.resize(steps + 1);
	probTree.resize(steps + 1);

	std::vector<double> finalPrices, finalProbs;

	for (int i = 0; i <= steps; i++) {
		priceTree[i].resize(i + 1);
		probTree[i].resize(i + 1);

		for (int j = 0; j <= i; j++) {
			priceTree[i][j] = S0 * pow(1 + change, i - j) * pow(1 - change, j);
			probTree[i][j] = static_cast<double>(binomialCoeff(i, j)) * pow(probUp, i - j) * pow(1 - probUp, j);
		}

		if (i == steps) {
			finalPrices = priceTree[i];
			finalProbs = probTree[i];
		}
	}
	return { finalPrices, finalProbs };
}

int BOPM::exportCSV(const std::string& filename) const {
	std::ofstream ofp(filename);
	if (!ofp) {
		std::cerr << "Error: cannot open " << filename << "\n";
		return 1;
	}

	for (int j = 0; j <= steps; j++) {
		for (int i = 0; i <= steps; i++) {
			if (j <= i) {
				ofp << std::fixed << std::setprecision(4)
					<< priceTree[i][j] << "(" << probTree[i][j] << ")";
			}
			if (i < steps) ofp << ",";
		}
		ofp << "\n";
	}
	std::cout << "The output is successfully exported to " << filename << "\n";
	ofp.close();
	return 0;
}

double BOPM::callPrice(const std::vector<double>& finalPrices,
					   const std::vector<double>& finalProbs,
					   double strike, double r, double freq) const {
	double sum = 0.0;
	for (size_t i = 0; i < finalPrices.size(); i++) {
		double payoff = std::max(finalPrices[i] - strike, 0.0);
		sum += payoff * finalProbs[i];
	}
	double discount = pow((1 + r / freq), finalPrices.size() - 1);
	return sum / discount;
}

double BOPM::putPrice(const std::vector<double>& finalPrices,
					  const std::vector<double>& finalProbs,
					  double strike, double r, double freq) const {
	double sum = 0.0;
	for (size_t i = 0; i < finalPrices.size(); i++) {
		double payoff = std::max(strike - finalPrices[i], 0.0);
		sum += payoff * finalProbs[i];
	}
	double discount = pow((1 + r / freq), finalPrices.size() - 1);
	return sum / discount;
}

void BOPM::printResults(const std::string& optionType,
						const std::vector<double>& finalPrices,
						const std::vector<double>& finalProbs,
						double strike, double price) const {
	std::cout << optionType << " Option\n";
	std::cout << std::setw(15) << "Final Price"
		<< std::setw(15) << "Payoff"
		<< std::setw(15) << "Probability"
		<< "\n";

	for (size_t i = 0; i < finalPrices.size(); i++) {
		double payoff = (optionType == "Call") ? std::max(finalPrices[i] - strike, 0.0) : std::max(strike - finalPrices[i], 0.0);
		std::cout << std::setw(15) << std::fixed << std::setprecision(4) << finalPrices[i]
			<< std::setw(15) << payoff
			<< std::setw(15) << finalProbs[i] << "\n";
	}

	std::cout << "\nThe price of the " << optionType << " option is " << price << "\n\n";
}

int main() {
	// parameters
	double initialPrice = 10.0;
	double nextStepPrice = 11.0;
	double probUp = 0.50;
	double frequencyPerYear = 3.0;
	double maturityInYear = 1.0;
	double strike = 10.0;
	double riskFreeRatePA = 0.08;

	BOPM model(initialPrice, nextStepPrice, probUp, frequencyPerYear, maturityInYear);

	// build tree
	auto finals = model.buildTree();
	model.exportCSV("bopm_output.csv");

	// call option
	double call = model.callPrice(finals.first, finals.second, strike, riskFreeRatePA, frequencyPerYear);
	model.printResults("Call", finals.first, finals.second, strike, call);

	// put option
	double put = model.putPrice(finals.first, finals.second, strike, riskFreeRatePA, frequencyPerYear);
	model.printResults("Put", finals.first, finals.second, strike, put);

	return 0;
}