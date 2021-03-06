/*********************************************************
  FileName     [ BotDiscover.h ]
  Description  [ discover bot based on anomalous windows ]
  Author       [ Cheng-Yeh (Gary) Chen ]
  Copyright    [ MIT ]
*********************************************************/
#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "util.h"

using namespace std;


class BotDiscover
{
public:
	BotDiscover();
	BotDiscover(const vector<bool>& anomaly, const vector< vector< vector<string> > >& timeList);
	~BotDiscover();
	
	// set function
	void setSCG(const vector< vector< vector<string> > >& timeList, const double tau = 100);
	void setSCG2(const double tau = 0.0);

	// public get function
	vector< vector<double> > get_SCG() const { return _SCG; }
	vector<string> get_ipList() const { return _ipList; }
	
	// conversion for specific use
	vector<double> convert_pivotalInteraction();
	
private:
	// filter
	bool degreeOneFilter(const set<SCG_Node*>& neighbor);
	void trimAnomalyList(vector<string>& removed);
	
	// node operation
	SCG_Node* newNode();
	void deleteNode(SCG_Node*& ptr);

	// statistic computation for SCGNode
	double mean(const vector<double>& inter);
	double deviation(const vector<double>& i, const double bar);
	double corelation_coefficient(const vector<double>& inter_i, const vector<double>& inter_j, const double bar_i, const double bar_j);

private:
	vector<bool> _anomaly;
	int _anomalyNumber;
	vector<string> _pivotList;
	map<string, SCG_Node*> _anomalyList;
	vector< vector<double> > _SCG;
	vector<string> _ipList;
};
