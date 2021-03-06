/*********************************************************
  FileName     [ BotDiscover.cpp ]
  Description  [ discover bot based on anomalous windows ]
  Author       [ Cheng-Yeh (Gary) Chen ]
  Copyright    [ MIT ]
*********************************************************/
#include <map>
#include <math.h>

#include "BotDiscover.h"

using namespace std;

extern globalArg args;

BotDiscover::BotDiscover()
{
	
}

BotDiscover::BotDiscover(const vector<bool>& anomaly, const vector< vector< vector<string> > >& timeList)
{
	_anomalyNumber = 0;
	_anomaly = anomaly;
	setSCG(timeList);
	setSCG2();
}

BotDiscover::~BotDiscover()
{

}

void
BotDiscover::setSCG(const vector< vector< vector<string> > >& timeList, const double tau)
{
	int count = 0;
	map<string, double> total;
	map< string, set<SCG_Node*> > neighbor;
	
	// compute total interaction
	for(size_t i = 0; i < _anomaly.size(); ++i){
		if(_anomaly[i]){
			++_anomalyNumber;
			for(size_t j = 0; j < timeList[i].size(); ++j){
				if(total.find(timeList[i][j][3]) == total.end())
					total[ timeList[i][j][3] ] = 1;
				else
					++total[ timeList[i][j][3] ];

				if(total.find(timeList[i][j][6]) == total.end())
					total[ timeList[i][j][6] ] = 1;
				else
					++total[ timeList[i][j][6] ];
			}
		}
	}
	
	// select pivots and create SCG_Node in _anomalyList
	cout << "pivot:";
	for(auto& x: total){
		if(x.second / _anomalyNumber > tau){
			_pivotList.push_back(x.first);
			if(!_anomalyList.emplace(x.first, newNode()).second)
				cout << "Error in BotDiscover::setpivot\n";
			_anomalyList[x.first] -> pivot = true;
			_anomalyList[x.first] -> total = total[x.first] / _anomalyNumber;
			cout << x.first << " ";
			cout << x.second / _anomalyNumber << endl;
		}
	}
	
	// create SCG_Node and compute interaction with pivots
	for(size_t i = 0; i < _anomaly.size(); ++i){
		if(_anomaly[i]){
			for(size_t j = 0; j < timeList[i].size(); ++j){
				for(int k = 0; k < 2; ++k){
					size_t k1 = k ? 6 : 3;
					if(_anomalyList.find(timeList[i][j][k1]) == _anomalyList.end()){
						_anomalyList.emplace(timeList[i][j][k1], newNode());
						_anomalyList[ timeList[i][j][k1] ] -> pivot = false;
						_anomalyList[ timeList[i][j][k1] ] -> total = total[ timeList[i][j][k1] ] / _anomalyNumber;

						neighbor[ timeList[i][j][k1] ] = set<SCG_Node*>();
					}
				}
				for(int k = 0; k < 2; ++k){
					size_t k1 = k ? 6 : 3, k2 = k ? 3 : 6;
					if(_anomalyList[ timeList[i][j][k1] ] -> pivot){
						++(_anomalyList[ timeList[i][j][k1] ] -> interaction)[i];
						++(_anomalyList[ timeList[i][j][k2] ] -> interaction)[i];
						
						neighbor[ timeList[i][j][k1] ].insert(_anomalyList[ timeList[i][j][k1] ]);
						neighbor[ timeList[i][j][k2] ].insert(_anomalyList[ timeList[i][j][k1] ]);
					}
				}
			}
		}
	}
	cout << "_anomalyList.size = " << _anomalyList.size();

	// print pivots and their total interaction
	for(auto& x: _anomalyList)
		if(x.second -> pivot)
			cout << x.first << ":" << mean(x.second -> interaction) << endl;
	
	// optional
	// delete nodes with degree less or equal to one
	vector<string> removed;
	for(auto& x: neighbor){
		if(degreeOneFilter(x.second)){
			deleteNode(_anomalyList[x.first]);
			removed.push_back(x.first);
		}
	}
	// delete node in _anomalyList
	trimAnomalyList(removed);
	
	// refresh id in _anomalyList
	count = 0;
	for(auto& x: _anomalyList)
		x.second -> id = count++;
		
	cout << "_anomalyList.size() after deleting: " << _anomalyList.size();
}

void
BotDiscover::setSCG2(const double tau)
{
	size_t debug = 0;
	vector< vector<double> > temp_SCG = vector< vector<double> >(_anomalyList.size(), vector<double>(_anomalyList.size(), 0));
	vector<string> temp_ipList = vector<string>(_anomalyList.size(), "\0");
	
	// construct SCG with respect to tau(threshold)
	for(map<string, SCG_Node*>::iterator it1 = _anomalyList.begin(); it1 != _anomalyList.end(); ++it1){
		if(debug % 100 == 0)cout << debug << endl;
		// record IP list
		temp_ipList[it1 -> second -> id] = it1 -> first;
		for(map<string, SCG_Node*>::iterator it2 = it1; it2 != _anomalyList.end(); ++it2){
			if(it1 == it2)continue;
			++debug;
			if(corelation_coefficient(it1 -> second -> interaction, it2 -> second -> interaction, it1 -> second -> total, it2 -> second -> total) > tau){
				//++debug;
				//(it1 -> second -> out_list).insert(it2 -> second);
				//it2 -> second -> in_list.insert(it1 -> second);
				temp_SCG[it1 -> second -> id][it2 -> second -> id] = 1;
				temp_SCG[it2 -> second -> id][it1 -> second -> id] = 1;
			}
		}
	}
	
	// record non-empty row and column
	vector<size_t> nonempty;
	for(size_t i = 0; i < temp_SCG.size(); ++i){
		int count = 0;
		for(size_t j = 0; j < temp_SCG.size(); ++j){
			count += temp_SCG[i][j];
		}
		if(count)nonempty.push_back(i);
	}
	
	cout << "final size = " << nonempty.size() << endl;
	
	// construct final SCG and ipList
	_SCG = vector< vector<double> >(nonempty.size(), vector<double>(nonempty.size(), 0));
	_ipList = vector<string>(nonempty.size(), "\0");
	for(size_t i = 0; i < _SCG.size(); ++i){
		_ipList[i] = temp_ipList[nonempty[i]];
		for(size_t j = 0; j < _SCG.size(); ++j){
			_SCG[i][j] = temp_SCG[nonempty[i]][nonempty[j]];
		}
	}
	
	cout << "SCG  :\n";
	for(size_t i = 0; i < _SCG.size(); ++i){
		for(size_t j = 0; j < _SCG.size(); ++j)
			cout << _SCG[i][j] << " ";
		cout << endl;
	}
}

vector<double> 
BotDiscover::convert_pivotalInteraction()
{
	vector<double> pivotal = vector<double>(_ipList.size(), 0);
	for(size_t i = 0; i < _ipList.size(); ++i){
		if(_anomalyList[_ipList[i]] -> pivot){
			for(auto& x: _anomalyList[_ipList[i]] -> out_list)
				if(x -> pivot)
					pivotal[i] += x -> total;
			for(auto& x: _anomalyList[_ipList[i]] -> in_list)
				if(x -> pivot)
					pivotal[i] += x -> total;
		}
		else{
			for(auto& x: _anomalyList[_ipList[i]] -> out_list){
				pivotal[i] += x -> total;
			}
			for(auto& x: _anomalyList[_ipList[i]] -> in_list){
			pivotal[i] += x -> total;
			}
		}
	}
	// normalization
	double maxi = *max_element(pivotal.begin(), pivotal.end());
	for(size_t i = 0 ; i < pivotal.size(); ++i){
		pivotal[i] /= maxi;
	}
	return pivotal;
}

bool
BotDiscover::degreeOneFilter(const set<SCG_Node*>& neighbor)
{

	return (neighbor.size() + neighbor.size() > 1) ? false : true;
}

void
BotDiscover::trimAnomalyList(vector<string>& removed)
{
	for(size_t i = 0; i < removed.size(); ++i){
		_anomalyList.erase(removed[i]);
	}
	removed.clear();
}

SCG_Node*
BotDiscover::newNode()
{
	SCG_Node* ptr = new SCG_Node;
	(ptr -> in_list) = set<SCG_Node*>();
	(ptr -> out_list) = set<SCG_Node*>();
	(ptr -> interaction).assign(args.windowNumber, 0);
	return ptr;
}

void
BotDiscover::deleteNode(SCG_Node*& ptr)
{	
	//for(size_t k = 0; k < _anomaly.size(); ++k){
	//	for(auto& x: ptr -> out_list[k])
	//		_anomalyList[x] -> in_list[k].erase(ip);
	//	for(auto& x: ptr -> in_list[k])
	//		_anomalyList[x] -> out_list[k].erase(ip);
	//}			
	delete ptr;
	ptr = 0;
}

double
BotDiscover::mean(const vector<double>& inter)
{
	double count = 0;
	for(size_t k = 0; k < inter.size(); ++k)
		if(_anomaly[k])
			count += inter[k];
	
	return count / _anomalyNumber;
}

double
BotDiscover::deviation(const vector<double>& inter, const double bar)
{
	double count = 0;
	for(size_t k = 0; k < inter.size(); ++k){
		if(_anomaly[k])
			count += pow( (inter[k] - bar), 2 );
	}
	return sqrt( count / (_anomalyNumber - 1) );
}

double
BotDiscover::corelation_coefficient(const vector<double>& inter_i, const vector<double>& inter_j, const double bar_i, const double bar_j)
{
	double count = 0;
	for(size_t k = 0; k < inter_i.size(); ++k)
		if(_anomaly[k]){
			count += (inter_i[k] - bar_i) * (inter_j[k] - bar_j);
		}
	return count / ( (_anomalyNumber - 1) * deviation(inter_i, bar_i) * deviation(inter_j, bar_j) );
}
