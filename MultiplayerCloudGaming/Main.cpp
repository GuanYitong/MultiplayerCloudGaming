#include "ServerAllocationProblem.h"
#include "MatchmakingProblem.h"

int main(int argc, char *argv[])
{
	/*double DELAY_BOUND_TO_G = 75, DELAY_BOUND_TO_R = 50, SESSION_SIZE = 10;
	if (argc >= 4)
	{
	DELAY_BOUND_TO_G = std::stod(argv[1]);
	DELAY_BOUND_TO_R = std::stod(argv[2]);
	SESSION_SIZE = std::stod(argv[3]);

	if (DELAY_BOUND_TO_G <= 0 || DELAY_BOUND_TO_R <= 0 || SESSION_SIZE <= 0)
	{
	printf("ERROR: invalid main() parameters\n");
	cin.get();
	return 0;
	}
	}*/

	/*ServerAllocationProblem*/
	/*ServerAllocationProblem::SimulateBasicProblem(75, 50, 10);
	ServerAllocationProblem::SimulateBasicProblem(75, 50, 50);
	ServerAllocationProblem::SimulateBasicProblem(150, 100, 10);
	ServerAllocationProblem::SimulateBasicProblem(150, 100, 50);*/

	/*MatchmakingProblem*/
	auto simulator = MatchmakingProblem::MaximumMatchingProblem();	
	for (int clientCount : { 25, 50, 100, 200, 400, 800 })
		for (int latencyThreshold : { 25, 50, 100 })
			simulator.Simulate(clientCount, latencyThreshold);

	return 0;
}