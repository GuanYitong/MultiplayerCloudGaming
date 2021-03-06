#include "ServerAllocationProblem.h"

namespace ServerAllocationProblem
{	
	void ResetEligibiltyCoverability(const vector<ClientType*> &clients, const vector<DatacenterType*> &datacenters)
	{
		for (auto c : clients)
		{
			//c->eligibleDatacenterList.clear();
			c->eligibleDatacenters.clear();
		}

		for (auto d : datacenters)
		{
			//d->coverableClientList.clear();
			d->coverableClients.clear();
		}
	}

	void ResetAssignment(const vector<ClientType*> &clients, const vector<DatacenterType*> &datacenters)
	{
		for (auto c : clients)
		{
			c->assignedDatacenterID = -1;
		}

		for (auto d : datacenters)
		{
			//d->assignedClientList.clear();
			d->assignedClients.clear();
		}
	}

	// matchmaking for basic problem
	// result: the datacenter for hosting the G-server, and a list of clients to be involved
	// return true if found
	bool Matchmaking4BasicProblem(vector<DatacenterType*> allDatacenters,
		vector<ClientType*> allClients,
		int &GDatacenterID,
		vector<ClientType*> &sessionClients,
		double SESSION_SIZE,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R)
	{
		ResetEligibiltyCoverability(allClients, allDatacenters);
		sessionClients.clear();

		random_shuffle(allDatacenters.begin(), allDatacenters.end()); // for randomizing each session's G-server selection process	

		for (auto Gdc : allDatacenters) // iterating all datacenters until we find an eligible G datacenter
		{
			random_shuffle(allClients.begin(), allClients.end()); // for randomizing each session's involved clients		

			for (auto client : allClients)
			{
				if ((int)sessionClients.size() == SESSION_SIZE) // reach the target session size
				{
					GDatacenterID = Gdc->id; // record the G datacenter id
					return true; // succeed
				}

				for (auto dc : allDatacenters)
				{
					if ((client->delayToDatacenter[dc->id] + dc->delayToDatacenter[Gdc->id]) <= DELAY_BOUND_TO_G
						&& client->delayToDatacenter[dc->id] <= DELAY_BOUND_TO_R)
					{
						//client->eligibleDatacenterList.push_back(tuple<int, double, double, double, double>(dc->id, client->delayToDatacenter[dc->id], dc->priceServer, dc->priceBandwidth * client->chargedTrafficVolume, dc->priceCombined));
						client->eligibleDatacenters.push_back(dc);

						//dc->coverableClientList.push_back(client->id);
						dc->coverableClients.push_back(client);
					}
				}

				if (!client->eligibleDatacenters.empty())
				{
					sessionClients.push_back(client); // put this client into the session 		
				}
			}

			// reset for next round of search		
			ResetEligibiltyCoverability(allClients, allDatacenters);
			sessionClients.clear();
		}

		return false;
	}

	// just to find if there are any eligible datacenters to open GS given the input (candidate datacenters, client group, delay bounds) 
	// record all of them if found
	// for general problem
	void SearchEligibleGDatacenter(vector<DatacenterType*> allDatacenters,
		vector<ClientType*> sessionClients,
		vector<DatacenterType*> &eligibleGDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R)
	{
		eligibleGDatacenters.clear();

		for (auto GDatacenter : allDatacenters)
		{
			bool isValidGDatacenter = true;

			for (auto client : sessionClients)
			{
				int numEligibleDC = 0;
				for (auto dc : allDatacenters)
				{
					if ((client->delayToDatacenter[dc->id] + dc->delayToDatacenter[GDatacenter->id]) <= DELAY_BOUND_TO_G &&
						client->delayToDatacenter[dc->id] <= DELAY_BOUND_TO_R)
					{
						numEligibleDC++;
					}
				}
				if (numEligibleDC < 1)
				{
					isValidGDatacenter = false; // found a client that has no eligible R-server locations given this G-server location, so this G-server location is invalid
					break;
				}
			}

			if (isValidGDatacenter)	eligibleGDatacenters.push_back(GDatacenter); // put it into the list				
		}
	}

	// result: a list of datacenters that are eligible for hosting the G-server, and a list of clients to be involved 
	// return true if found
	// for general problem
	bool Matchmaking4GeneralProblem(vector<DatacenterType*> allDatacenters,
		vector<ClientType*> allClients,
		vector<ClientType*> &sessionClients,
		vector<DatacenterType*> &eligibleGDatacenters,
		double SESSION_SIZE,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R)
	{
		ResetEligibiltyCoverability(allClients, allDatacenters);
		sessionClients.clear();
		eligibleGDatacenters.clear();

		int initialGDatacenter; // just for satisfying MatchmakingBasicProblem's parameters
		if (Matchmaking4BasicProblem(allDatacenters, allClients, initialGDatacenter, sessionClients, SESSION_SIZE, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R))
		{
			SearchEligibleGDatacenter(allDatacenters, sessionClients, eligibleGDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
		}

		return (!sessionClients.empty() && !eligibleGDatacenters.empty());
	}

	// used inside each strategy function
	// for general problem
	void SimulationSetup4GeneralProblem(DatacenterType *GDatacenter, vector<ClientType*> sessionClients, vector<DatacenterType*> allDatacenters, double DELAY_BOUND_TO_G, double DELAY_BOUND_TO_R)
	{
		ResetEligibiltyCoverability(sessionClients, allDatacenters);

		for (auto client : sessionClients) // find eligible datacenters for each client and coverable clients for each dc
		{
			for (auto dc : allDatacenters)
			{
				if ((client->delayToDatacenter[dc->id] + dc->delayToDatacenter[GDatacenter->id]) <= DELAY_BOUND_TO_G && client->delayToDatacenter[dc->id] <= DELAY_BOUND_TO_R)
				{
					//client->eligibleDatacenterList.push_back(tuple<int, double, double, double, double>(dc->id, client->delayToDatacenter[dc->id], dc->priceServer, dc->priceBandwidth, dc->priceCombined)); // record eligible dc's id, delay, priceServer, priceBandwidth, priceCombined
					client->eligibleDatacenters.push_back(dc);

					//dc->coverableClientList.push_back(client->id);
					dc->coverableClients.push_back(client);
				}
			}
		}
	}

	// include G-server's cost into the total cost according to the group size
	// used inside the following strategy functions
	// for general problem only
	void IncludeGServerCost(DatacenterType *GDatacenter, double SESSION_SIZE, bool includingGServerCost, double &tempTotalCost)
	{
		if (includingGServerCost)
		{
			if (SESSION_SIZE <= 20)
				tempTotalCost += GDatacenter->priceServer / 8;
			else if (SESSION_SIZE <= 40)
				tempTotalCost += GDatacenter->priceServer / 4;
			else if (SESSION_SIZE <= 80)
				tempTotalCost += GDatacenter->priceServer / 2;
			else
				tempTotalCost += GDatacenter->priceServer;
		}
	}

	// function to get the solution output 
	// return <cost_total, cost_server, cost_bandwidth, capacity_wastage, average_delay>
	tuple<double, double, double, double, double> GetSolutionOutput(
		vector<DatacenterType*> allDatacenters, 
		double serverCapacity,
		vector<ClientType*> sessionClients,
		int GDatacenterID)
	{
		double costServer = 0, costBandwidth = 0, numberServers = 0;
		for (auto dc : allDatacenters)
		{
			//dc->openServerCount = ceil(dc->assignedClientList.size() / serverCapacity);
			dc->openServerCount = ceil(double(dc->assignedClients.size()) / serverCapacity);
			numberServers += dc->openServerCount;
			costServer += dc->priceServer * dc->openServerCount;
			double totalChargedTrafficVolume = 0;
			for (auto client : dc->assignedClients)
			{
				totalChargedTrafficVolume += client->chargedTrafficVolume;
			}
			costBandwidth += dc->priceBandwidth * totalChargedTrafficVolume;
		}

		double totalDelay = 0;
		for (auto client : sessionClients)
		{
			totalDelay += client->delayToDatacenter[client->assignedDatacenterID] + allDatacenters.at(client->assignedDatacenterID)->delayToDatacenter[GDatacenterID];
		}

		return tuple<double, double, double, double, double>(
			costServer + costBandwidth,
			costServer,
			costBandwidth,
			(numberServers * serverCapacity - sessionClients.size()) / sessionClients.size(),
			totalDelay / sessionClients.size());
	}

	// return true if and only if all clients are assigned and each client is assigned to one dc
	bool CheckIfAllClientsExactlyAssigned(vector<ClientType*> sessionClients, vector<DatacenterType*> allDatacenters)
	{
		for (auto c : sessionClients)
		{
			bool validAssignment = false;
			for (auto edc : c->eligibleDatacenters)
			{
				if (c->assignedDatacenterID == edc->id)
				{
					validAssignment = true;
					break;
				}
			}
			if (!validAssignment)
				return false;
		}

		int totalAssignedClientCount = 0;
		for (auto d : allDatacenters)
		{
			//totalAssignedClientCount += (int)d->assignedClientList.size();
			totalAssignedClientCount += (int)d->assignedClients.size();
		}
		return (totalAssignedClientCount == (int)sessionClients.size());
	}

	bool Initialize(string dataDirectory, vector<ClientType*> &allClients, vector<DatacenterType*> &allDatacenters)
	{
		/* temporary stuff */
		vector<vector<double>> ClientToDatacenterDelayMatrix;
		vector<vector<double>> InterDatacenterDelayMatrix;
		vector<double> priceServerList;
		vector<double> priceBandwidthList;
		
		/* client-to-dc latency data */
		auto strings_read = ReadDelimitedTextFileIntoVector(dataDirectory + "dc_to_pl_rtt.csv", ',', true);
		if (strings_read.empty())
		{	
			printf("ERROR: empty file!\n");
			cin.get();
			return false;
		}
		for (auto row : strings_read)
		{
			vector<double> ClientToDatacenterDelayMatrixOneRow;
			for (size_t col = 1; col < row.size(); col++)
			{
				ClientToDatacenterDelayMatrixOneRow.push_back(stod(row.at(col)) / 2);
			}
			ClientToDatacenterDelayMatrix.push_back(ClientToDatacenterDelayMatrixOneRow);
		}

		/*dc-to-dc latency data*/
		strings_read = ReadDelimitedTextFileIntoVector(dataDirectory + "dc_to_dc_rtt.csv", ',', true);
		if (strings_read.empty())
		{
			printf("ERROR: empty file!\n");
			cin.get();
			return false;
		}
		for (auto row : strings_read)
		{
			vector<double> InterDatacenterDelayMatrixOneRow;
			for (size_t col = 1; col < row.size(); col++)
			{
				InterDatacenterDelayMatrixOneRow.push_back(stod(row.at(col)) / 2);
			}
			InterDatacenterDelayMatrix.push_back(InterDatacenterDelayMatrixOneRow);
		}
		const int totalClientCount = int(ClientToDatacenterDelayMatrix.size());

		/* bandwidth and server price data */
		strings_read = ReadDelimitedTextFileIntoVector(dataDirectory + "dc_pricing_bandwidth_server.csv", ',', true);
		if (strings_read.empty())
		{
			printf("ERROR: empty file!\n");
			cin.get();
			return false;
		}
		for (auto row : strings_read)
		{
			priceBandwidthList.push_back(stod(row.at(1)));
			priceServerList.push_back(stod(row.at(2))); // 2: g2.8xlarge, 3: g2.2xlarge
		}
		const int totalDatacenterCount = int(ClientToDatacenterDelayMatrix.front().size());

		/* creating clients */
		allClients.clear();
		for (int i = 0; i < totalClientCount; i++)
		{
			ClientType* client = new ClientType(i);
			client->chargedTrafficVolume = 2;
			for (int j = 0; j < totalDatacenterCount; j++)
			{
				client->delayToDatacenter[j] = ClientToDatacenterDelayMatrix.at(i).at(j);
			}
			allClients.push_back(client);
		}
		printf("%d clients created according to the input latency data file\n", int(allClients.size()));

		/* create datacenters */
		allDatacenters.clear();
		for (int i = 0; i < totalDatacenterCount; i++)
		{
			DatacenterType* dc = new DatacenterType(i);
			dc->priceServer = priceServerList.at(i);
			dc->priceBandwidth = priceBandwidthList.at(i);
			for (auto client : allClients)
			{
				dc->delayToClient[client->id] = client->delayToDatacenter[dc->id];
			}
			for (int j = 0; j < totalDatacenterCount; j++)
			{
				dc->delayToDatacenter[j] = InterDatacenterDelayMatrix.at(i).at(j);
			}
			allDatacenters.push_back(dc);
		}
		printf("%d datacenters created according to the input latency data file\n", int(allDatacenters.size()));

		return true;
	}

	void WriteCostWastageDelayData(int STRATEGY_COUNT, vector<double> SERVER_CAPACITY_LIST, double SESSION_COUNT,
		vector<vector<vector<tuple<double, double, double, double, double>>>>& outcomeAtAllSessions,
		string dataDirectory, string experimentSettings)
	{
		// record total cost to files
		vector<vector<vector<double>>> costTotalStrategyCapacitySession;
		for (int i = 0; i < STRATEGY_COUNT; i++) // columns
		{
			vector<vector<double>> costTotalCapacitySession;
			for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++) // rows
			{
				vector<double> costTotalSession;
				for (int k = 0; k < SESSION_COUNT; k++) // entries
				{
					//costTotalSession.push_back(get<0>(outcomeAtAllSessions.at(k).at(j).at(i))); // for each session k of this capacity of this strategy (raw data)
					costTotalSession.push_back(get<0>(outcomeAtAllSessions.at(k).at(j).at(i)) / get<0>(outcomeAtAllSessions.at(k).at(j).front())); // normalized by the LB
				}
				costTotalCapacitySession.push_back(costTotalSession); // for each capacity j of this strategy
			}
			costTotalStrategyCapacitySession.push_back(costTotalCapacitySession); // for each strategy i
		}

		/*StreamWriter^ costTotalMeanFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "costTotalMean");
		StreamWriter^ costTotalStdFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "costTotalStd");*/
		ofstream costTotalMeanFile(dataDirectory + "Output\\" + experimentSettings + "_" + "costTotalMean.csv");
		ofstream costTotalStdFile(dataDirectory + "Output\\" + experimentSettings + "_" + "costTotalStd.csv");
		for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
		{
			for (int i = 0; i < STRATEGY_COUNT; i++)
			{
				costTotalMeanFile << GetMeanValue(costTotalStrategyCapacitySession.at(i).at(j)) << ",";
				costTotalStdFile << GetStdValue(costTotalStrategyCapacitySession.at(i).at(j)) << ",";
			}
			costTotalMeanFile << "\n";
			costTotalStdFile << "\n";
		}
		costTotalMeanFile.close();
		costTotalStdFile.close();

		// record capacity wastage ratio
		vector<vector<vector<double>>> capacityWastageStrategyCapacitySession;
		for (int i = 0; i < STRATEGY_COUNT; i++)
		{
			vector<vector<double>> capacityWastageCapacitySession;
			for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
			{
				vector<double> capacityWastageSession;
				for (int k = 0; k < SESSION_COUNT; k++)
				{
					capacityWastageSession.push_back(get<3>(outcomeAtAllSessions.at(k).at(j).at(i))); // for each session k of this capacity of this strategy
				}
				capacityWastageCapacitySession.push_back(capacityWastageSession); // for each capacity j of this strategy
			}
			capacityWastageStrategyCapacitySession.push_back(capacityWastageCapacitySession); // for each strategy i
		}

		/*StreamWriter^ capacityWastageMeanFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "capacityWastageMean");
		StreamWriter^ capacityWastageStdFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "capacityWastageStd");*/
		ofstream capacityWastageMeanFile(dataDirectory + "Output\\" + experimentSettings + "_" + "capacityWastageMean.csv");
		ofstream capacityWastageStdFile(dataDirectory + "Output\\" + experimentSettings + "_" + "capacityWastageStd.csv");
		for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
		{
			for (int i = 0; i < STRATEGY_COUNT; i++)
			{
				capacityWastageMeanFile << GetMeanValue(capacityWastageStrategyCapacitySession.at(i).at(j)) << ",";
				capacityWastageStdFile << GetStdValue(capacityWastageStrategyCapacitySession.at(i).at(j)) << ",";
			}
			capacityWastageMeanFile << "\n";
			capacityWastageStdFile << "\n";
		}
		capacityWastageMeanFile.close();
		capacityWastageStdFile.close();

		// record average delay (client to G-server)
		vector<vector<vector<double>>> averageDelayStrategyCapacitySession;
		for (int i = 0; i < STRATEGY_COUNT; i++)
		{
			vector<vector<double>> averageDelayCapacitySession;
			for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
			{
				vector<double> averageDelaySession;
				for (int k = 0; k < SESSION_COUNT; k++)
				{
					averageDelaySession.push_back(get<4>(outcomeAtAllSessions.at(k).at(j).at(i))); // for each session k of this capacity of this strategy
				}
				averageDelayCapacitySession.push_back(averageDelaySession); // for each capacity j of this strategy
			}
			averageDelayStrategyCapacitySession.push_back(averageDelayCapacitySession); // for each strategy i
		}

		/*StreamWriter^ averageDelayMeanFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "averageDelayMean");
		StreamWriter^ averageDelayStdFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "averageDelayStd");*/
		ofstream averageDelayMeanFile(dataDirectory + "Output\\" + experimentSettings + "_" + "averageDelayMean.csv");
		ofstream averageDelayStdFile(dataDirectory + "Output\\" + experimentSettings + "_" + "averageDelayStd.csv");
		for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
		{
			for (int i = 0; i < STRATEGY_COUNT; i++)
			{
				averageDelayMeanFile << GetMeanValue(averageDelayStrategyCapacitySession.at(i).at(j)) << + ",";
				averageDelayStdFile << GetStdValue(averageDelayStrategyCapacitySession.at(i).at(j)) << + ",";
			}
			averageDelayMeanFile << "\n";
			averageDelayStdFile << "\n";
		}
		averageDelayMeanFile.close();
		averageDelayStdFile.close();
	}

	void SimulateBasicProblem(double DELAY_BOUND_TO_G, double DELAY_BOUND_TO_R, double SESSION_SIZE, double SESSION_COUNT)
	{
		//srand((unsigned)time(nullptr)); // using current time as the seed for random_shuffle() and rand(), otherwise, they will generate the same sequence of random numbers in every run

		auto t0 = clock(); // start time	

		string dataDirectory = ".\\Data\\ProblemBasic\\";
		vector<ClientType*> allClients;
		vector<DatacenterType*> allDatacenters;
		if (!Initialize(dataDirectory, allClients, allDatacenters))
		{
			printf("ERROR: simulation initialization failed!\n");
			cin.get();
			return;
		}
		
		// data structures for storing results
		vector<vector<vector<tuple<double, double, double, double, double>>>> outcomeAtAllSessions;
		vector<vector<vector<double>>> computationAtAllSessions;
		vector<int> eligibleRDatacenterCount;
		vector<int> GDatacenterIDAtAllSessions;
		vector<double> matchmakingTimeAtAllSessions;

		vector<double> SERVER_CAPACITY_LIST = { 2, 4, 6, 8 };
		int STRATEGY_COUNT = 8;

		for (int sessionID = 1; sessionID <= SESSION_COUNT; sessionID++)
		{
			vector<ClientType*> sessionClients;
			int GDatacenterID;

			auto matchmakingStartTime = clock();
			bool isFeasibleSession = Matchmaking4BasicProblem(allDatacenters, allClients, GDatacenterID, sessionClients, SESSION_SIZE, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			matchmakingTimeAtAllSessions.push_back(difftime(clock(), matchmakingStartTime));
			if (!isFeasibleSession)
			{
				printf("--------------------------------------------------------------------\n");
				printf("ERROR: infeasible session\n");
				printf("total elapsed time: %d seconds\n", (int)(difftime(clock(), t0) / 1000));
				cin.get();
				return;
			}
			GDatacenterIDAtAllSessions.push_back(GDatacenterID);
			for (auto client : sessionClients)
			{
				//eligibleRDatacenterCount.push_back((int)client->eligibleDatacenterList.size());
				eligibleRDatacenterCount.push_back((int)client->eligibleDatacenters.size());
			}

			printf("------------------------------------------------------------------------\n");
			printf("delay bounds: (%d, %d) session size: %d session: %d\n", (int)DELAY_BOUND_TO_G, (int)DELAY_BOUND_TO_R, (int)SESSION_SIZE, sessionID);
			printf("executing strategies\n");

			vector<vector<tuple<double, double, double, double, double>>> outcomeAtOneSession; // per session
			vector<vector<double>> computationAtOneSession; // per session				

			for (auto serverCapacity : SERVER_CAPACITY_LIST)
			{
				//for (auto dc : allDatacenters)
				//{
				//	dc->priceCombined = (dc->priceServer / serverCapacity) + dc->priceBandwidth; // dependent on server capacity
				//}

				vector<tuple<double, double, double, double, double>> outcomeAtOneCapacity; // per capacity
				vector<double> computationAtOneCapacity; // per capacity

				for (int strategyID = 1; strategyID <= STRATEGY_COUNT; strategyID++)
				{
					tuple<double, double, double, double, double> outcome;	
					auto timePoint = clock();
					switch (strategyID)
					{
					case 1:
						outcome = Alg_LB(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 2:
						outcome = Alg_RANDOM(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 3:
						outcome = Alg_NEAREST(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 4:
						outcome = Alg_LSP(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 5:
						outcome = Alg_LBP(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 6:
						outcome = Alg_LCP(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 7:
						outcome = Alg_LCW(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					case 8:
						outcome = Alg_LAC(sessionClients, allDatacenters, serverCapacity, GDatacenterID);
						break;
					default:
						outcome = tuple<double, double, double, double, double>(0, 0, 0, 0, 0);
						break;
					}		
					computationAtOneCapacity.push_back((double)(clock() - timePoint)); // record computation time per strategy
					outcomeAtOneCapacity.push_back(outcome); // record outcome per strategy

					if (!CheckIfAllClientsExactlyAssigned(sessionClients, allDatacenters))
					{
						printf("Something wrong with client-to-datacenter assignment!\n");
						cin.get();
						return;
					}

					cout << "*";
				} // end of strategy loop

				outcomeAtOneSession.push_back(outcomeAtOneCapacity); // record outcome per capacity
				computationAtOneSession.push_back(computationAtOneCapacity); // record computation time per capacity	

				cout << endl;
			} // end of capacity loop

			outcomeAtAllSessions.push_back(outcomeAtOneSession); // per session size
			computationAtAllSessions.push_back(computationAtOneSession); // per session size

			printf("end of executing strategies\n");
		}

		/*******************************************************************************************************/

		string experimentSettings = std::to_string((int)DELAY_BOUND_TO_G) + "_" + std::to_string((int)DELAY_BOUND_TO_R) + "_" + std::to_string((int)SESSION_SIZE);

		// record cost, wastage and delay
		WriteCostWastageDelayData(STRATEGY_COUNT, SERVER_CAPACITY_LIST, SESSION_COUNT, outcomeAtAllSessions, dataDirectory, experimentSettings);

		// record eligible RDatacenter count
		//StreamWriter^ eligibleRDatacenterCountFile = gcnew StreamWriter(dataDirectory + "Output\\eligibleRDatacenterCount\\" + experimentSettings + "_" + "eligibleRDatacenterCount");
		/*ofstream eligibleRDatacenterCountFile(dataDirectory + "Output\\eligibleRDatacenterCount\\" + experimentSettings + "_" + "eligibleRDatacenterCount.csv");
		for (auto it : eligibleRDatacenterCount)
		{
			eligibleRDatacenterCountFile << it << "\n";
		}
		eligibleRDatacenterCountFile.close();*/

		// record GDatancenter
		//StreamWriter^ GDatacenterIDFile = gcnew StreamWriter(dataDirectory + "Output\\GDatacenterID\\" + experimentSettings + "_" + "GDatacenterID");
		/*ofstream GDatacenterIDFile(dataDirectory + "Output\\GDatacenterID\\" + experimentSettings + "_" + "GDatacenterID.csv");
		for (auto it : GDatacenterIDAtAllSessions)
		{
			GDatacenterIDFile << it << "\b";
		}
		GDatacenterIDFile.close();*/

		//// record matchmaking's time
		//StreamWriter^ matchmakingTimeFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "matchmakingTime");
		//for (auto it : matchmakingTimeAtAllSessions)
		//{
		//	matchmakingTimeFile->WriteLine(it);
		//}
		//matchmakingTimeFile->Close();	

		// record computation time
		vector<vector<vector<double>>> computationStrategyCapacitySession;
		for (int i = 0; i < STRATEGY_COUNT; i++)
		{
			vector<vector<double>> computationCapacitySession;
			for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
			{
				vector<double> computationSession;
				for (int k = 0; k < SESSION_COUNT; k++)
				{
					computationSession.push_back(computationAtAllSessions.at(k).at(j).at(i)); // for each session k of this capacity of this strategy
				}
				computationCapacitySession.push_back(computationSession); // for each capacity j of this strategy
			}
			computationStrategyCapacitySession.push_back(computationCapacitySession); // for each strategy i
		}
		/*StreamWriter^ computationMeanFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "computationMean");
		StreamWriter^ computationStdFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "computationStd");*/
		ofstream computationMeanFile(dataDirectory + "Output\\" + experimentSettings + "_" + "computationMean.csv");
		ofstream computationStdFile(dataDirectory + "Output\\" + experimentSettings + "_" + "computationStd.csv");
		for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
		{
			for (int i = 0; i < STRATEGY_COUNT; i++)
			{
				computationMeanFile << GetMeanValue(computationStrategyCapacitySession.at(i).at(j)) << ",";
				computationStdFile << GetStdValue(computationStrategyCapacitySession.at(i).at(j)) << ",";
			}
			computationMeanFile << "\n";
			computationStdFile << "\n";
		}
		computationMeanFile.close();
		computationStdFile.close();

		/*******************************************************************************************************/

		printf("------------------------------------------------------------------------\n");
		printf("total elapsed time: %d seconds\n", (int)(difftime(clock(), t0) / 1000)); // elapsed time of the process
		//cin.get();
		return;
	}

	void SimulateGeneralProblem(double DELAY_BOUND_TO_G, double DELAY_BOUND_TO_R, double SESSION_SIZE, double SESSION_COUNT)
	{
		//srand((unsigned)time(nullptr)); // using current time as the seed for random_shuffle() and rand(), otherwise, they will generate the same sequence of random numbers in every run

		auto t0 = clock(); // start time	

		string dataDirectory = ".\\Data\\ProblemGeneral\\";
		vector<ClientType*> allClients;
		vector<DatacenterType*> allDatacenters;
		if (!Initialize(dataDirectory, allClients, allDatacenters))
		{
			printf("ERROR: simulation initialization failed!\n");
			cin.get();
			return;
		}

		// data structures for storing results
		vector<vector<vector<tuple<double, double, double, double, double>>>> outcomeAtAllSessions;
		vector<vector<vector<double>>> computationAtAllSessions;
		vector<vector<vector<int>>> finalGDatacenterAtAllSessions;

		vector<int> eligibleGDatacenterCountAtAllSessions;
		vector<double> matchmakingTimeAtAllSessions;

		map<int, double> serverCountPerDC4LCP;
		map<int, double> serverCountPerDC4LCW;
		map<int, double> serverCountPerDC4LAC;
		for (auto dc : allDatacenters)
		{
			serverCountPerDC4LCP[dc->id] = 0;
			serverCountPerDC4LCW[dc->id] = 0;
			serverCountPerDC4LAC[dc->id] = 0;
		}

		vector<double> SERVER_CAPACITY_LIST = { 2, 4, 6, 8 };

		int STRATEGY_COUNT = 8;

		for (int sessionID = 1; sessionID <= SESSION_COUNT; sessionID++)
		{
			vector<ClientType*> sessionClients;
			vector<DatacenterType*> eligibleGDatacenters;

			auto matchmakingStartTime = clock();

			bool isFeasibleSession = Matchmaking4GeneralProblem(allDatacenters, allClients, sessionClients, eligibleGDatacenters, SESSION_SIZE, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);

			matchmakingTimeAtAllSessions.push_back(difftime(clock(), matchmakingStartTime));

			if (!isFeasibleSession)
			{
				printf("*********************************************************************\n");
				printf("ERROR: infeasible session\n");
				printf("total elapsed time: %d seconds\n", (int)(difftime(clock(), t0) / 1000));
				cin.get();
				return;
			}

			printf("------------------------------------------------------------------------");
			printf("delay bounds: (%d, %d)   session size: %d   session: %d\n", (int)DELAY_BOUND_TO_G, (int)DELAY_BOUND_TO_R, (int)SESSION_SIZE, sessionID);
			printf("start of one session\n");

			vector<vector<tuple<double, double, double, double, double>>> outcomeAtOneSession; // per session
			vector<vector<double>> computationAtOneSession; // per session
			vector<vector<int>> finalGDatacenterAtOneSession; // per session		

			for (auto serverCapacity : SERVER_CAPACITY_LIST)
			{
				//for (auto dc : allDatacenters)
				//{
				//	dc->priceCombined = (dc->priceServer / serverCapacity) + dc->priceBandwidth; // dependent on server capacity
				//}

				vector<tuple<double, double, double, double, double>> outcomeAtOneCapacity; // per capacity		
				vector<double> computationAtOneCapacity; // per capacity
				vector<int> finalGDatacenterAtOneCapacity; // per capacity	

				for (int strategyID = 1; strategyID <= STRATEGY_COUNT; strategyID++)
				{
					tuple<double, double, double, double, double> outcome;				
					int finalGDatacenter;
					auto timePoint = clock();
					switch (strategyID)
					{
					case 1:
						outcome = Alg_LB(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						break;
					case 2:
						outcome = Alg_RANDOM(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						break;
					case 3:
						outcome = Alg_NEAREST(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						break;
					case 4:
						outcome = Alg_LSP(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						break;
					case 5:
						outcome = Alg_LBP(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						break;
					case 6:
						outcome = Alg_LCP(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						if (8 == serverCapacity)
						{
							for (auto dc : allDatacenters)
							{
								serverCountPerDC4LCP[dc->id] += dc->openServerCount;
							}
						}
						break;
					case 7:
						outcome = Alg_LCW(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						if (8 == serverCapacity)
						{
							for (auto dc : allDatacenters)
							{
								serverCountPerDC4LCW[dc->id] += dc->openServerCount;
							}
						}
						break;
					case 8:
						outcome = Alg_LAC(eligibleGDatacenters, finalGDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R, serverCapacity);
						if (8 == serverCapacity)
						{
							for (auto dc : allDatacenters)
							{
								serverCountPerDC4LAC[dc->id] += dc->openServerCount;
							}
						}
						break;
					default:
						outcome = tuple<double, double, double, double, double>(0, 0, 0, 0, 0);
						break;
					}
					computationAtOneCapacity.push_back((double)(clock() - timePoint)); // per strategy
					outcomeAtOneCapacity.push_back(outcome); // per strategy					
					finalGDatacenterAtOneCapacity.push_back(finalGDatacenter); // per strategy

					if (!CheckIfAllClientsExactlyAssigned(sessionClients, allDatacenters))
					{
						printf("Something wrong with the assignment!\n");
						cin.get();
						return;
					}			

					cout << "*";
				} // end of strategy loop

				outcomeAtOneSession.push_back(outcomeAtOneCapacity); // per capacity
				computationAtOneSession.push_back(computationAtOneCapacity); // per capacity	
				finalGDatacenterAtOneSession.push_back(finalGDatacenterAtOneCapacity); // per capacity

				cout << endl;
			} // end of capacity loop

			outcomeAtAllSessions.push_back(outcomeAtOneSession); // per session
			computationAtAllSessions.push_back(computationAtOneSession); // per session
			finalGDatacenterAtAllSessions.push_back(finalGDatacenterAtOneSession); // per session

			eligibleGDatacenterCountAtAllSessions.push_back((int)eligibleGDatacenters.size()); // per session

			printf("end of one session\n");
		} // end of session iteration

		/*******************************************************************************************************/

		string experimentSettings = std::to_string((int)DELAY_BOUND_TO_G) + "_" + std::to_string((int)DELAY_BOUND_TO_R) + "_" + std::to_string((int)SESSION_SIZE);

		// record cost, wastage and delay
		WriteCostWastageDelayData(STRATEGY_COUNT, SERVER_CAPACITY_LIST, SESSION_COUNT, outcomeAtAllSessions, dataDirectory, experimentSettings);

		// record server count at each datacenter in all sessions
		//StreamWriter^ serverCountPerDCFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "serverCountPerDC");
		ofstream serverCountPerDCFile(dataDirectory + "Output\\" + experimentSettings + "_" + "serverCountPerDC.csv");
		for (auto dc : allDatacenters)
		{
			serverCountPerDCFile << serverCountPerDC4LCP[dc->id] << ",";
		}
		serverCountPerDCFile << "\n";
		for (auto dc : allDatacenters)
		{
			serverCountPerDCFile << serverCountPerDC4LCW[dc->id] << ",";
		}
		serverCountPerDCFile << "\n";
		for (auto dc : allDatacenters)
		{
			serverCountPerDCFile << serverCountPerDC4LAC[dc->id] << ",";
		}
		serverCountPerDCFile.close();

		// record final G datacenter
		//StreamWriter^ finalGDatacenterFile = gcnew StreamWriter(dataDirectory + "Output\\finalGDatacenterID\\" + experimentSettings + "_" + "finalGDatacenterID");
		/*ofstream finalGDatacenterFile(dataDirectory + "Output\\finalGDatacenterID\\" + experimentSettings + "_" + "finalGDatacenterID.csv");
		for (size_t i = 0; i < finalGDatacenterAtAllSessions.size(); i++)
		{
			for (size_t j = 0; j < finalGDatacenterAtAllSessions.at(i).size(); j++)
			{
				for (size_t k = 0; k < finalGDatacenterAtAllSessions.at(i).at(j).size(); k++)
				{
					finalGDatacenterFile << finalGDatacenterAtAllSessions.at(i).at(j).at(k) << ",";
				}
				finalGDatacenterFile << "\n";
			}
		}
		finalGDatacenterFile.close();*/

		// record eligible GDatacenter count
		//StreamWriter^ eligibleGDatacenterCountFile = gcnew StreamWriter(dataDirectory + "Output\\eligibleGDatacenterCount\\" + experimentSettings + "_" + "eligibleGDatacenterCount");
		ofstream eligibleGDatacenterCountFile(dataDirectory + "Output\\eligibleGDatacenterCount\\" + experimentSettings + "_" + "eligibleGDatacenterCount.csv");
		/*for (auto it : eligibleGDatacenterCountAtAllSessions)
		{
			eligibleGDatacenterCountFile << it << "\n";
		}
		eligibleGDatacenterCountFile.close();*/

		// record computation time
		vector<vector<vector<double>>> computationStrategyCapacitySession;
		for (int i = 0; i < STRATEGY_COUNT; i++)
		{
			vector<vector<double>> computationCapacitySession;
			for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
			{
				vector<double> computationSession;
				for (int k = 0; k < SESSION_COUNT; k++)
				{
					computationSession.push_back(computationAtAllSessions.at(k).at(j).at(i)); // for each session k of this capacity of this strategy
				}
				computationCapacitySession.push_back(computationSession); // for each capacity j of this strategy
			}
			computationStrategyCapacitySession.push_back(computationCapacitySession); // for each strategy i
		}
		/*StreamWriter^ computationMeanFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "computationMean");
		StreamWriter^ computationStdFile = gcnew StreamWriter(dataDirectory + "Output\\" + experimentSettings + "_" + "computationStd");*/
		ofstream computationMeanFile(dataDirectory + "Output\\" + experimentSettings + "_" + "computationMean.csv");
		ofstream computationStdFile(dataDirectory + "Output\\" + experimentSettings + "_" + "computationStd.csv");
		for (size_t j = 0; j < SERVER_CAPACITY_LIST.size(); j++)
		{
			for (int i = 0; i < STRATEGY_COUNT; i++)
			{
				computationMeanFile << GetMeanValue(computationStrategyCapacitySession.at(i).at(j)) << " ";
				computationStdFile << GetStdValue(computationStrategyCapacitySession.at(i).at(j)) << " ";
			}
			computationMeanFile << "\n";
			computationStdFile << "\n";
		}
		computationMeanFile.close();
		computationStdFile.close();

		/*******************************************************************************************************/

		printf("------------------------------------------------------------------------\n");
		printf("total elapsed time: %d seconds\n", (int)(difftime(clock(), t0) / 1000)); // elapsed time of the process
		//cin.get();
		return;
	}

	// Lower-Bound (LB)
	// for basic problem
	tuple<double, double, double, double, double> Alg_LB(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		for (auto client : sessionClients)
		{
			//client->assignedDatacenterID = get<0>(*min_element(client->eligibleDatacenterList.begin(), client->eligibleDatacenterList.end(), EligibleDCComparatorByPriceCombined));
			client->assignedDatacenterID = client->eligibleDatacenters.front()->id;
			for (auto edc : client->eligibleDatacenters)
			{
				double priceCombinedPrevious = allDatacenters.at(client->assignedDatacenterID)->priceServer / serverCapacity + allDatacenters.at(client->assignedDatacenterID)->priceBandwidth * client->chargedTrafficVolume;
				double priceCombinedCurrent = edc->priceServer / serverCapacity + edc->priceBandwidth * client->chargedTrafficVolume;
				if (priceCombinedCurrent < priceCombinedPrevious)
				{
					client->assignedDatacenterID = edc->id;
				}
				else if (priceCombinedCurrent == priceCombinedPrevious)
				{
					if (client->delayToDatacenter.at(edc->id) < client->delayToDatacenter.at(client->assignedDatacenterID))
					{
						client->assignedDatacenterID = edc->id;
					}
				}
			}

			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id);
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		double costServer = 0, costBandwidth = 0, numberServers = 0;
		for (auto dc : allDatacenters)
		{
			//dc->openServerCount = dc->assignedClientList.size() / serverCapacity;
			dc->openServerCount = double(dc->assignedClients.size()) / serverCapacity;
			numberServers += dc->openServerCount;
			costServer += dc->priceServer * dc->openServerCount;
			double totalChargedTrafficVolume = 0;
			for (auto client : dc->assignedClients)
			{
				totalChargedTrafficVolume += client->chargedTrafficVolume;
			}
			costBandwidth += dc->priceBandwidth * totalChargedTrafficVolume;
		}

		double totalDelay = 0;
		for (auto client : sessionClients)
		{
			totalDelay += client->delayToDatacenter[client->assignedDatacenterID] + allDatacenters.at(client->assignedDatacenterID)->delayToDatacenter[GDatacenterID];
		}

		return tuple<double, double, double, double, double>(
			costServer + costBandwidth,
			costServer,
			costBandwidth,
			0,
			totalDelay / sessionClients.size());
	}

	// Lower-Bound (LB)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LB(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R); // initilization		
			auto tempOutcome = Alg_LB(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);
			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);
			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
			}
		}

		finalGDatacenter = tempFinalGDatacenter;
		return finalOutcome;
	}

	// Random-Assignment
	// for basic problem
	tuple<double, double, double, double, double> Alg_RANDOM(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		for (auto client : sessionClients) // choose a dc for each client
		{
			//client->assignedDatacenterID = get<0>(client->eligibleDatacenterList.at(rand() % (int)client->eligibleDatacenterList.size()));
			vector<DatacenterType*> copyEligibleDatacenters(client->eligibleDatacenters);
			std::random_shuffle(copyEligibleDatacenters.begin(), copyEligibleDatacenters.end());
			client->assignedDatacenterID = copyEligibleDatacenters.front()->id;
		
			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id); // add this client to the chosen dc's assigned client list
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Random-Assignment
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_RANDOM(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R); // initilization		
			auto tempOutcome = Alg_RANDOM(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);
			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);
			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
			}
		}

		finalGDatacenter = tempFinalGDatacenter;
		return finalOutcome;
	}

	// Nearest-Assignment
	// for basic problem
	tuple<double, double, double, double, double> Alg_NEAREST(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		for (auto client : sessionClients)
		{
			//client->assignedDatacenterID = get<0>(*min_element(client->eligibleDatacenterList.begin(), client->eligibleDatacenterList.end(), EligibleDCComparatorByDelay)); // choose the nearest eligible dc for each client		
			client->assignedDatacenterID = client->eligibleDatacenters.front()->id;
			for (auto edc : client->eligibleDatacenters)
			{
				if (client->delayToDatacenter[edc->id] < client->delayToDatacenter[client->assignedDatacenterID])
				{
					client->assignedDatacenterID = edc->id;
				}
			}

			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id); // add this client to the chosen dc's assigned client list
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Nearest-Assignment
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_NEAREST(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R); // initilization				
			auto tempOutcome = Alg_NEAREST(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);
			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);
			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
			}
		}

		finalGDatacenter = tempFinalGDatacenter;
		return finalOutcome;
	}

	// Lowest-Server-Price-Datacenter-Assignment (LSP)
	// for basic problem
	tuple<double, double, double, double, double> Alg_LSP(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		for (auto client : sessionClients)
		{
			//client->assignedDatacenterID = get<0>(*min_element(client->eligibleDatacenterList.begin(), client->eligibleDatacenterList.end(), EligibleDCComparatorByPriceServer));
			client->assignedDatacenterID = client->eligibleDatacenters.front()->id;
			for (auto edc : client->eligibleDatacenters)
			{
				if (edc->priceServer < allDatacenters.at(client->assignedDatacenterID)->priceServer)
				{
					client->assignedDatacenterID = edc->id;
				}
				else if (edc->priceServer == allDatacenters.at(client->assignedDatacenterID)->priceServer)
				{
					if (client->delayToDatacenter.at(edc->id) < client->delayToDatacenter.at(client->assignedDatacenterID))
					{
						client->assignedDatacenterID = edc->id;
					}
				}
			}

			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id); // add this client to the chosen dc's assigned client list
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Lowest-Bandwidth-Price-Datacenter-Assignment (LBP)
	// for basic problem
	tuple<double, double, double, double, double> Alg_LBP(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		auto clock_begin = clock();
		for (auto client : sessionClients)
		{
			//client->assignedDatacenterID = get<0>(*min_element(client->eligibleDatacenterList.begin(), client->eligibleDatacenterList.end(), EligibleDCComparatorByPriceBandwidth));
			client->assignedDatacenterID = client->eligibleDatacenters.front()->id;
			for (auto edc : client->eligibleDatacenters)
			{
				if (edc->priceBandwidth < allDatacenters.at(client->assignedDatacenterID)->priceBandwidth)
				{
					client->assignedDatacenterID = edc->id;
				}
				else if (edc->priceBandwidth == allDatacenters.at(client->assignedDatacenterID)->priceBandwidth)
				{
					if (client->delayToDatacenter.at(edc->id) < client->delayToDatacenter.at(client->assignedDatacenterID))
					{
						client->assignedDatacenterID = edc->id;
					}
				}
			}

			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id); // add this client to the chosen dc's assigned client list
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Lowest-Combined-Price-Datacenter-Assignment (LCP)
	// for basic problem
	tuple<double, double, double, double, double> Alg_LCP(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		ResetAssignment(sessionClients, allDatacenters);

		for (auto client : sessionClients)
		{
			//client->assignedDatacenterID = get<0>(*min_element(client->eligibleDatacenterList.begin(), client->eligibleDatacenterList.end(), EligibleDCComparatorByPriceCombined));
			client->assignedDatacenterID = client->eligibleDatacenters.front()->id;
			for (auto edc : client->eligibleDatacenters)
			{
				double priceCombinedPrevious = allDatacenters.at(client->assignedDatacenterID)->priceServer / serverCapacity + allDatacenters.at(client->assignedDatacenterID)->priceBandwidth * client->chargedTrafficVolume;
				double priceCombinedCurrent = edc->priceServer / serverCapacity + edc->priceBandwidth * client->chargedTrafficVolume;
				if (priceCombinedCurrent < priceCombinedPrevious)
				{
					client->assignedDatacenterID = edc->id;
				}
				else if (priceCombinedCurrent == priceCombinedPrevious)
				{
					if (client->delayToDatacenter.at(edc->id) < client->delayToDatacenter.at(client->assignedDatacenterID))
					{
						client->assignedDatacenterID = edc->id;
					}
				}
			}

			//allDatacenters.at(client->assignedDatacenterID)->assignedClientList.push_back(client->id); // add this client to the chosen dc's assigned client list
			allDatacenters.at(client->assignedDatacenterID)->assignedClients.push_back(client);
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Lowest-Server-Price-Datacenter-Assignment (LSP)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LSP(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;

		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		map<int, double> finalServerCountPerDC; // newly added

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			auto tempOutcome = Alg_LSP(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);

			map<int, double> tempServerCountPerDC; // newly added
			for (auto dc : allDatacenters) // newly added
			{
				tempServerCountPerDC[dc->id] = dc->openServerCount;
			}

			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);

			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;

				finalOutcome = tempOutcome;

				tempFinalGDatacenter = GDatacenter->id;

				finalServerCountPerDC = tempServerCountPerDC; // newly added
			}
		}

		finalGDatacenter = tempFinalGDatacenter;

		for (auto dc : allDatacenters) // newly added
		{
			dc->openServerCount = finalServerCountPerDC[dc->id];
		}

		return finalOutcome;
	}

	// Lowest-Bandwidth-Price-Datacenter-Assignment (LBP)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LBP(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;

		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			auto tempOutcome = Alg_LBP(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);
			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);
			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
			}
		}

		finalGDatacenter = tempFinalGDatacenter;
		return finalOutcome;
	}

	// Lowest-Combined-Price-Datacenter-Assignment (LCP)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LCP(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			auto tempOutcome = Alg_LCP(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);
			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);
			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
			}
		}

		finalGDatacenter = tempFinalGDatacenter;
		return finalOutcome;
	}

	// Lowest-Capacity-Wastage-Assignment (LCW)
	// if server capacity < 2, reduce to LCP
	// for basic problem
	tuple<double, double, double, double, double> Alg_LCW(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		if (serverCapacity < 2)	return Alg_LCP(sessionClients, allDatacenters, serverCapacity, GDatacenterID);

		ResetAssignment(sessionClients, allDatacenters);

		while (true)
		{
			vector<DatacenterType*> candidateDatacenters;
			candidateDatacenters.clear();
			for (auto dc : allDatacenters)
			{
				dc->unassignedCoverableClients.clear(); // reset for new iteration

				for (auto client : dc->coverableClients)
				{
					if (client->assignedDatacenterID < 0)
					{
						dc->unassignedCoverableClients.push_back(client);
					}
				}

				if (!dc->unassignedCoverableClients.empty())
				{
					candidateDatacenters.push_back(dc);
				}
			}
			if (candidateDatacenters.empty()) // indicating that no more datacenters that have unassigned coverable clients
			{
				break; // terminate
			}

			DatacenterType* nextDC = candidateDatacenters.front(); // initialization
			for (auto dc : candidateDatacenters)
			{
				double utilization_dc; // compute utilization for the current dc
				int unassignedClientCount_dc = (int)dc->unassignedCoverableClients.size();
				if (unassignedClientCount_dc % (int)serverCapacity == 0)
					utilization_dc = 1;
				else
					utilization_dc = (double)(unassignedClientCount_dc % (int)serverCapacity) / serverCapacity;

				double utilization_nextDC; // compute utilization for the previously selected dc (the one with highest utilization so far)
				int unassignedClientCount_nextDC = (int)nextDC->unassignedCoverableClients.size();
				if (unassignedClientCount_nextDC % (int)serverCapacity == 0)
					utilization_nextDC = 1;
				else
					utilization_nextDC = (double)(unassignedClientCount_nextDC % (int)serverCapacity) / serverCapacity;

				// choose the one with higher projected utilization	
				// if two utilizations tie, select the one with lower server price
				if (utilization_dc > utilization_nextDC || (utilization_dc == utilization_nextDC && dc->priceServer < nextDC->priceServer))
				{
					nextDC = dc;
				}
			}

			for (auto client : nextDC->unassignedCoverableClients) // client-to-datacenter assignment
			{
				client->assignedDatacenterID = nextDC->id;
				//nextDC->assignedClientList.push_back(client->id);
				nextDC->assignedClients.push_back(client);
			}
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Lowest-Capacity-Wastage-Assignment (LCW)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LCW(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		map<int, double> finalServerCountPerDC; // newly added

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			auto tempOutcome = Alg_LCW(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);

			map<int, double> tempServerCountPerDC; // newly added
			for (auto dc : allDatacenters) // newly added
			{
				tempServerCountPerDC[dc->id] = dc->openServerCount;
			}

			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);

			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
				finalServerCountPerDC = tempServerCountPerDC; // newly added
			}
		}

		finalGDatacenter = tempFinalGDatacenter;

		for (auto dc : allDatacenters) // newly added
		{
			dc->openServerCount = finalServerCountPerDC[dc->id];
		}

		return finalOutcome;
	}

	// Lowest-Average-Cost-Assignment (LAC)
	// Idea: open exactly one server at each iteration, and where to open the server is determined based on the average cost contributed by all clients that are to be assigned to this server
	// if server capacity < 2, reduce to LCP
	// for basic problem
	tuple<double, double, double, double, double> Alg_LAC(
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double serverCapacity,
		int GDatacenterID)
	{
		if (serverCapacity < 2)	return Alg_LCP(sessionClients, allDatacenters, serverCapacity, GDatacenterID);

		ResetAssignment(sessionClients, allDatacenters);

		while (true)
		{
			// update for the new iteration
			vector<DatacenterType*> candidateDatacenters;
			candidateDatacenters.clear();
			for (auto dc : allDatacenters)
			{
				dc->unassignedCoverableClients.clear();

				for (auto client : dc->coverableClients)
				{
					if (client->assignedDatacenterID < 0) { dc->unassignedCoverableClients.push_back(client); }
				}

				if (!dc->unassignedCoverableClients.empty()) { candidateDatacenters.push_back(dc); }
			}

			// all clients are assigned, so terminate the iteration
			if (candidateDatacenters.empty())
			{
				break;
			}

			// compute the average cost per client if opening a server in each dc		
			for (auto dc : candidateDatacenters)
			{				
				if (dc->unassignedCoverableClients.empty())
				{
					dc->averageCostPerClient = INT_MAX;
				}
				else if (dc->unassignedCoverableClients.size() <= serverCapacity)
				{					
					double avgServerCostPerClient = dc->priceServer / dc->unassignedCoverableClients.size();
					double totalChargedTrafficVolume = 0;
					for (auto client : dc->unassignedCoverableClients)
					{
						totalChargedTrafficVolume += client->chargedTrafficVolume;
					}
					double avgBandwidthCostPerClient = (dc->priceBandwidth * totalChargedTrafficVolume) / dc->unassignedCoverableClients.size();
					
					dc->averageCostPerClient = avgServerCostPerClient + avgBandwidthCostPerClient;
				}
				else
				{
					double avgServerCostPerClient = dc->priceServer / serverCapacity;
					double totalChargedTrafficVolume = 0;
					for (auto client : dc->unassignedCoverableClients)
					{
						totalChargedTrafficVolume += client->chargedTrafficVolume;
					}
					double avgBandwidthCostPerClient = (dc->priceBandwidth * totalChargedTrafficVolume) / dc->unassignedCoverableClients.size();

					dc->averageCostPerClient = avgServerCostPerClient + avgBandwidthCostPerClient;
				}
			}

			//assign at most (serverCapacity) unassignedCoverable clients to the dc with the lowest averageCostPerClient
			//DatacenterType* nextDC = *min_element(candidateDatacenters.begin(), candidateDatacenters.end(), DCComparatorByAverageCostPerClient);
			auto nextDC = candidateDatacenters.front();
			for (auto cdc : candidateDatacenters)
			{
				if (cdc->averageCostPerClient < nextDC->averageCostPerClient)
				{
					nextDC = cdc;
				}
			}
			int unassignedCoverableClientCountNextDC = (int)nextDC->unassignedCoverableClients.size();
			int numberOfClientsToBeAssigned = (unassignedCoverableClientCountNextDC <= serverCapacity) ? unassignedCoverableClientCountNextDC : (int)serverCapacity;
			for (int i = 0; i < numberOfClientsToBeAssigned; i++)
			{
				nextDC->unassignedCoverableClients.at(i)->assignedDatacenterID = nextDC->id;
				//nextDC->assignedClientList.push_back(nextDC->unassignedCoverableClients.at(i)->id);
				nextDC->assignedClients.push_back(nextDC->unassignedCoverableClients.at(i));
			}
		}

		return GetSolutionOutput(allDatacenters, serverCapacity, sessionClients, GDatacenterID);
	}

	// Lowest-Average-Cost-Assignment (LAC)
	// overloaded for general problem
	tuple<double, double, double, double, double> Alg_LAC(
		vector<DatacenterType*> eligibleGDatacenters,
		int &finalGDatacenter,
		const vector<ClientType*> &sessionClients,
		const vector<DatacenterType*> &allDatacenters,
		double DELAY_BOUND_TO_G,
		double DELAY_BOUND_TO_R,
		double serverCapacity,
		bool includingGServerCost)
	{
		tuple<double, double, double, double, double> finalOutcome;
		double totalCost = INT_MAX;
		int tempFinalGDatacenter = eligibleGDatacenters.front()->id;

		map<int, double> finalServerCountPerDC; // newly added

		for (auto GDatacenter : eligibleGDatacenters)
		{
			SimulationSetup4GeneralProblem(GDatacenter, sessionClients, allDatacenters, DELAY_BOUND_TO_G, DELAY_BOUND_TO_R);
			auto tempOutcome = Alg_LAC(sessionClients, allDatacenters, serverCapacity, GDatacenter->id);
			double tempTotalCost = get<0>(tempOutcome);

			map<int, double> tempServerCountPerDC; // newly added
			for (auto dc : allDatacenters) // newly added
			{
				tempServerCountPerDC[dc->id] = dc->openServerCount;
			}

			//IncludeGServerCost(GDatacenter, (int)sessionClients.size(), includingGServerCost, tempTotalCost);

			if (tempTotalCost < totalCost) // choose the smaller cost
			{
				totalCost = tempTotalCost;
				finalOutcome = tempOutcome;
				tempFinalGDatacenter = GDatacenter->id;
				finalServerCountPerDC = tempServerCountPerDC; // newly added
			}
		}

		finalGDatacenter = tempFinalGDatacenter;

		for (auto dc : allDatacenters) // newly added
		{
			dc->openServerCount = finalServerCountPerDC[dc->id];
		}

		return finalOutcome;
	}
}