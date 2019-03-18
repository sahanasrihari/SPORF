#ifndef urfTree_h
#define urfTree_h
#include "../../../baseFunctions/fpBaseNode.h"
#include "unprocessedURFNode.h"
#include <vector>
#include <map>
#include <assert.h>
#include <random>

namespace fp{

	template <typename T>
		class urfTree
		{
			protected:
				float OOBAccuracy;
				float correctOOB;
				float totalOOB;
				std::vector< fpBaseNode<T, int> > tree;
				std::vector< unprocessedURFNode<T> > nodeQueue;
				std::vector< unprocessedURFNode<T> > leafNodes;

			public:
				urfTree() : OOBAccuracy(-1.0),correctOOB(0),totalOOB(0){}

				void loadFirstNode(){
					nodeQueue.emplace_back(fpSingleton::getSingleton().returnNumObservations());
				}

				inline bool shouldProcessNode(){
					if(nodeQueue.back().returnNodeImpurity()==0){
						return false;
					}
					if(nodeQueue.back().returnInSampleSize() <= fpSingleton::getSingleton().returnMinParent()){
						
return false;
					}
					return true;
				}

				inline float returnOOB(){
					return correctOOB/totalOOB;
				}

				inline int returnLastNodeID(){
					return tree.size()-1;
				}

				inline void linkParentToChild(){
					if(nodeQueue.back().returnIsLeftNode()){
						tree[nodeQueue.back().returnParentID()].setLeftValue(returnLastNodeID());
					}else{
						tree[nodeQueue.back().returnParentID()].setRightValue(returnLastNodeID());
					}
				}


				inline int returnMaxDepth(){
					int maxDepth=0;
					for(auto nodes : tree){
						if(maxDepth < nodes.returnDepth()){
							maxDepth = nodes.returnDepth();
						}
					}
					return maxDepth;
				}


				inline int returnNumLeafNodes(){
					int numLeafNodes=0;
					for(auto nodes : tree){
						if(!nodes.isInternalNode()){
							++numLeafNodes;
						}
					}
					return numLeafNodes;
				}

				inline void updateSimMat(std::map<int, std::map<int, int> > &simMat){
					for(auto nodes : leafNodes){
						stratifiedInNodeClassIndicesUnsupervised* obsI = nodes.returnObsIndices();
						std::vector<int> leafObs;
						leafObs = obsI->returnInSampsVec();
						auto siz = leafObs.size();
						if (siz <= 0)
							return;
						for(unsigned int i = 0; i < siz-1; ++i) {
							for (unsigned int j=i+1; j<siz; ++j) {
								#pragma omp critical
								{
								auto exists = simMat[leafObs[i]].emplace(leafObs[j], 1);
								if(!exists.second)
									simMat[leafObs[i]][leafObs[j]]++;
								}
							}
						}
					}
				}

				inline int returnLeafDepthSum(){
					int leafDepthSums=0;
					for(auto nodes : tree){
						if(!nodes.isInternalNode()){
							leafDepthSums += nodes.returnDepth();
						}
					}
					return leafDepthSums;
				}


				inline void setAsLeaf(){
					tree.back().setClass(nodeQueue.back().returnMaxClass());
					tree.back().setDepth(nodeQueue.back().returnDepth());
				}


				inline void checkOOB(){
					totalOOB += nodeQueue.back().returnOutSampleSize();
					correctOOB += nodeQueue.back().returnOutSampleError(tree.back().returnClass());
				}


				inline void makeWholeNodeALeaf(){
					tree.emplace_back();
					linkParentToChild();
					setAsLeaf();
					checkOOB();
					leafNodes.emplace_back(nodeQueue.back());
					nodeQueue.pop_back();
				}

				void printTree(){
					for(auto nd : tree){
						nd.printNode();
					}
				}

				inline void createNodeInTree(){
					tree.emplace_back();
					linkParentToChild();
					tree.back().setCutValue(nodeQueue.back().returnBestCutValue());
					tree.back().setFeatureValue(nodeQueue.back().returnBestFeature());
					tree.back().setDepth(nodeQueue.back().returnDepth());
				}


				inline void makeNodeInternal(){
					createNodeInTree();
					createChildren();
				}

				inline bool isLeftNode(){
					return true;
				}

				inline bool isRightNode(){
					return false;
				}

				inline void createChildren(){
					nodeQueue.back().moveDataLeftOrRight();

					stratifiedInNodeClassIndicesUnsupervised* leftIndices = nodeQueue.back().returnLeftIndices();
					stratifiedInNodeClassIndicesUnsupervised* rightIndices = nodeQueue.back().returnRightIndices();

					assert(leftIndices->returnInSampleSize() > 0);
					assert(rightIndices->returnInSampleSize() > 0);

					int childDepth = nodeQueue.back().returnDepth()+1;

					nodeQueue.pop_back();

					nodeQueue.emplace_back(returnLastNodeID(),childDepth, isLeftNode());
					nodeQueue.back().loadIndices(leftIndices);

					nodeQueue.emplace_back(returnLastNodeID(),childDepth, isRightNode());
					nodeQueue.back().loadIndices(rightIndices);
				}


				inline void findTheBestSplit(){
					nodeQueue.back().findBestSplit();
				}


				inline bool noGoodSplitFound(){
					return nodeQueue.back().returnBestFeature() == -1;
				}


				inline void processANode(){
					nodeQueue.back().setupNode();
					if(shouldProcessNode()){
						findTheBestSplit();
						if(noGoodSplitFound()){
							makeWholeNodeALeaf();
						}else{
							makeNodeInternal();
						}
					}else{
						makeWholeNodeALeaf();
					}
				}

				inline void processNodes(){
					while(!nodeQueue.empty()){
						processANode();
					}
				}

				inline void growTree(){
					loadFirstNode();
					processNodes();
				}

		};

}//fp
#endif //urfTree_h