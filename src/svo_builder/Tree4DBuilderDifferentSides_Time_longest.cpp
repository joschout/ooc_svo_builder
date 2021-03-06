#include "Tree4DBuilderDifferentSides_Time_longest.h"
#include <cassert>
#include "svo_builder_util.h"
#include "octree_io.h"



Tree4DBuilderDifferentSides_Time_longest::Tree4DBuilderDifferentSides_Time_longest(): 
	Tree4DBuilderDifferentSides_Interface(0, 0, 0, 0, 0, 0, false, string()),
	nbOfQueuesOf16Nodes(0), nbOfQueuesOf2Nodes(0)
{
}

Tree4DBuilderDifferentSides_Time_longest::Tree4DBuilderDifferentSides_Time_longest(
	std::string base_filename, size_t gridsize_S, size_t gridsize_T, bool generate_levels):
	Tree4DBuilderDifferentSides_Interface(gridsize_S, gridsize_T, 0, 0, 0, 0, generate_levels, base_filename),
	nbOfQueuesOf16Nodes(0), nbOfQueuesOf2Nodes(0)
{
}

void Tree4DBuilderDifferentSides_Time_longest::initializeBuilder()
{
	svo_io_output_timer.start();	// TIMING
	std::unique_ptr<TreeNodeWriterCppStyle> nWriter(new TreeNodeWriterCppStyle(base_filename));
	nodeWriter = std::move(nWriter);

	std::unique_ptr<TreeDataWriterCppStyle> dWriter(new TreeDataWriterCppStyle(base_filename));
	dataWriter = std::move(dWriter);
	svo_io_output_timer.stop();		// TIMING
	svo_algorithm_timer.start();	// TIMING

	/*
	gridsize_t = 2^y
	gridsize_s = 2^x  with y > x

	#vox = 2^(3x+y) ~> 2^(4 + D)    met D = y - x

	maxDepth = log2( max(gridsize_s, gridsize_t) ) ~> log2(gridsize_t) = y
	totalNbOfQueues = maxDepth + 1  ~> y + 1

	nbOfQueuesOf16Nodes = log2(gridsize_s) = x
	nbOfQueuesOf2Nodes = totalNbOfQueues - nbOfQueuesOf16Nodes ~> y + 1 - x = D + 1
	*/

	/*
	 MAAR WAT ALS y <= x?

	 gridsize_t = 2^y
	 gridsize_s = 2^x  with y <= x

	 #vox = 2^(3x+y)

	 maxDepth = log2( max(gridsize_s, gridsize_t) ) ~> log2(gridsize_S) = X
	 totalNbOfQueues = maxDepth + 1  ~> y + 1

	 nbOfQueuesOf16Nodes = log2(gridsize_s) = x
	 nbOfQueuesOf2Nodes = totalNbOfQueues - nbOfQueuesOf16Nodes ~> y + 1 - x = D + 1

	*/



	// Setup building variables
	maxDepth = log2(static_cast<int>(max(gridsize_S, gridsize_T)));
	totalNbOfQueues = maxDepth + 1;


	nbOfQueuesOf16Nodes = log2(static_cast<int>(gridsize_S));
	nbOfQueuesOf2Nodes = totalNbOfQueues - nbOfQueuesOf16Nodes;

	queuesOfMax2.resize(nbOfQueuesOf2Nodes);
	queuesOfMax16.resize(nbOfQueuesOf16Nodes);

	//nodeQueues.resize(maxDepth + 1);
	for (int i = 0; i < nbOfQueuesOf2Nodes; i++)
	{
		queuesOfMax2[i].reserve(2);
	}
	for (int i = 0; i < nbOfQueuesOf16Nodes; i++)
	{
		queuesOfMax16[i].reserve(16);
	}


	svo_algorithm_timer.stop();		// TIMING
	svo_total_timer.stop();			// TIMING
	if (data_out)
	{
		stringstream out;
		out
			<< "tree building - number of 2-element queues: " << nbOfQueuesOf2Nodes << endl
			<< "tree building - number of 16-element queues: " << nbOfQueuesOf16Nodes << endl
			<< "tree building - total number of queues: " << totalNbOfQueues << endl;
		data_writer_ptr->writeToFile(out.str());
	}
	svo_total_timer.start();		// TIMING
	svo_algorithm_timer.start();	// TIMING

	// Fill data arrays
	calculateMaxMortonCode();

	svo_algorithm_timer.stop();		// TIMING
	svo_io_output_timer.start();	// TIMING
	dataWriter->writeVoxelData(VoxelData());// first data point is NULL

#ifdef BINARY_VOXELIZATION
	VoxelData voxelData = VoxelData(0, vec3(), vec3(1.0, 1.0, 1.0)); // We store a simple white voxel in case of Binary voxelization
	dataWriter->writeVoxelData(voxelData);  // all leafs will refer to this
#endif
	svo_io_output_timer.stop();		// TIMING
	svo_algorithm_timer.stop();		// TIMING
}

void Tree4DBuilderDifferentSides_Time_longest::push_backNodeToQueueAtDepth(int depth, Node4D& node)
{
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if(depth <= nbOfQueuesOf2Nodes - 1) 
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
	{
		queuesOfMax2[depth].push_back(node);
	}else 
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]
	{
		queuesOfMax16[depth - nbOfQueuesOf2Nodes].push_back(node);
	}
}

bool Tree4DBuilderDifferentSides_Time_longest::isQueueFilled(int depth)
{
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if (depth <= nbOfQueuesOf2Nodes - 1) {
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
		if(queuesOfMax2[depth].size() == 2)
		{
			return true;
		} else {
			return false;
		}
	} else {
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]

		if(queuesOfMax16[depth - nbOfQueuesOf2Nodes].size() == 16)
		{
			return true;
		} else {
			return false;
		}	
	}
}

bool Tree4DBuilderDifferentSides_Time_longest::isQueueEmpty(int depth)
{
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if (depth <= nbOfQueuesOf2Nodes - 1) {
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
		if (queuesOfMax2[depth].size() == 0)
		{
			return true;
		}
		else {
			return false;
		}
	}
	else {
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]

		if (queuesOfMax16[depth - nbOfQueuesOf2Nodes].size() == 0)
		{
			return true;
		}
		else {
			return false;
		}
	}
}

/*
IMPORTANT NOTE:
This method expects the queue on the given depth to be completely full.
It is UNSAFE to call this method when the queue is not full.
It should be checked if the queue is full before calling this method.
*/
bool Tree4DBuilderDifferentSides_Time_longest::doesQueueContainOnlyEmptyNodes(int depth) {
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if (depth <= nbOfQueuesOf2Nodes - 1)
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
	{
		return doesQueueContainOnlyEmptyNodes(queuesOfMax2[depth], 2);
	}
	else
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]
	{
		return doesQueueContainOnlyEmptyNodes(queuesOfMax16[depth - nbOfQueuesOf2Nodes], 16);
	}
}

/*
Groups the nodes in the given queue.

PRECONDITION: The queue at the given depth is filled with nodes.
When calling this method, we know that some of the nodes in this queue are NOT empty leaf nodes.

RETURNS: the parent node of the nodes in this queue.

Calling this method will write the non-empty child nodes to disk.
It sets the child pointers to the non-empty child nodes in the returned parent node.
*/
Node4D Tree4DBuilderDifferentSides_Time_longest::groupNodesAtDepth(int depth)
{
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if (depth <= nbOfQueuesOf2Nodes - 1)
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
	{
		return groupNodesOfMax2(queuesOfMax2[depth]);
	}
	else
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]
	{
		return groupNodesOfMax16(queuesOfMax16[depth - nbOfQueuesOf2Nodes]);
	}
}

/*
Groups the nodes in the given queue.

PRECONDITION: The queue at the given depth is filled with nodes.
When calling this method, we know that some of the nodes in this queue are NOT empty leaf nodes.

RETURNS: the parent node of the nodes in this queue.

Calling this method will write the non-empty child nodes to disk.
It sets the child pointers to the non-empty child nodes in the returned parent node.
*/
Node4D Tree4DBuilderDifferentSides_Time_longest::groupNodesOfMax2(const QueueOfNodes &queueOfMax2)
{
	Node4D parent = Node4D();
	bool first_stored_child = true;

	for (int indexOfCurrentChildNode = 0; indexOfCurrentChildNode < 2; indexOfCurrentChildNode++) {
		Node4D currentChildNode = queueOfMax2[indexOfCurrentChildNode];
		if (!currentChildNode.isNull()) {
			writeNodeToDiskAndSetOffsetOfParent_Max2NodesInQueue(parent, first_stored_child, indexOfCurrentChildNode, currentChildNode);
		}
		else {
			setChildrenOffsetsForNodeWithMax2Children(parent, indexOfCurrentChildNode, NOCHILD);
		}
	}

	// SIMPLE LEVEL CONSTRUCTION
	if (generate_levels) {
		VoxelData voxelData = VoxelData();
		float notnull = 0.0f;
		for (int i = 0; i < 2; i++) { // this node has no data: need to refine
			if (!queueOfMax2[i].isNull()){
				notnull++;
			}
			voxelData.color += queueOfMax2[i].data_cache.color;
			voxelData.normal += queueOfMax2[i].data_cache.normal;
		}
		voxelData.color = voxelData.color / notnull;
		vec3 tonormalize = (vec3)(voxelData.normal / notnull);
		voxelData.normal = normalize(tonormalize);
		// set it in the parent node
		svo_algorithm_timer.stop();		// TIMING
		svo_io_output_timer.start();	// TIMING
		parent.data = dataWriter->writeVoxelData(voxelData);
		svo_io_output_timer.stop();		// TIMING
		svo_algorithm_timer.start();	// TIMING
		parent.data_cache = voxelData;
	}
	return parent;
}

void Tree4DBuilderDifferentSides_Time_longest::writeNodeToDiskAndSetOffsetOfParent_Max2NodesInQueue(Node4D& parent, bool& first_stored_child, int indexOfCurrentChildNode, Node4D currentChildNode)
{
	//store the node on disk
	svo_algorithm_timer.stop();		// TIMING
	svo_io_output_timer.start();	// TIMING
	size_t positionOfChildOnDisk = nodeWriter->writeNode4D_(currentChildNode);
	svo_io_output_timer.stop();		// TIMING
	svo_algorithm_timer.start();	// TIMING
	
	if (first_stored_child) {
		parent.children_base = positionOfChildOnDisk;
		setChildrenOffsetsForNodeWithMax2Children(parent, indexOfCurrentChildNode, 0);
		first_stored_child = false;	
	}
	else { //NOTE: we can have only two nodes, we know this is the second node
		char offset = (char)(positionOfChildOnDisk - parent.children_base);
		setChildrenOffsetsForNodeWithMax2Children(parent, indexOfCurrentChildNode, offset);	
	}
}


/*
Set in the node 'parent' the offset of its child node.
The offset is 'offset'. 
'indexInQueue' is the index of the child in the queue we are currently grouping.
*/
void Tree4DBuilderDifferentSides_Time_longest::setChildrenOffsetsForNodeWithMax2Children(Node4D &parent, int indexInQueue, char offset)
{
	if (indexInQueue == 0)
	{
		for (int i = 0; i < 8; i++) {
			parent.children_offset[i] = offset;
		}
	}
	else { // indexInQueue == 1
		for (int i = 8; i < 16; i++) {
			parent.children_offset[i] = offset;
		}
	}
}

void Tree4DBuilderDifferentSides_Time_longest::clearQueueAtDepth(int depth)
{
	assert(depth >= 0);
	assert(depth <= maxDepth);

	if (depth <= nbOfQueuesOf2Nodes - 1)
		//depth in [0, nbOfQueuesOf2nodes - 1]
		//index in queuesOfMax2 : [0, nbOfQueuesOf2nodes - 1]
	{
		queuesOfMax2[depth].clear();
	}
	else
		//depth in [nbOfQueuesOf2nodes, totalNbOfQueues - 1]    (remember: maxDepth = totalNbOfQueues - 1
		//index in queuesOfMax16 : [0, nbOfQueuesOf16nodes - 1]
	{
		queuesOfMax16[depth - nbOfQueuesOf2Nodes].clear();
	}
}

Node4D Tree4DBuilderDifferentSides_Time_longest::getRootNode()
{
	if ( nbOfQueuesOf2Nodes > 0)
	{
		return queuesOfMax2[0][0];
	}
	else
	{
		return queuesOfMax16[0][0];
	}
}

size_t Tree4DBuilderDifferentSides_Time_longest::nbOfEmptyVoxelsAddedByAddingAnEmptyNodeAtDepth(int depth)
{
//	if(depth >= nbOfQueuesOf2Nodes - 1) // depth >= y - x
//	{
//		return pow(16.0, maxDepth - depth);
//	}else{
//		//  nbOFNodesToAdd = 2^(4x + D) = pow(gridsize_S, 4) * 2^D
//		size_t leftFactor = pow(gridsize_S, 4.0);
//		size_t D = maxDepth - nbOfQueuesOf16Nodes - depth; //= nbOfQueuesOf2 - 1 - depth
//		size_t nbOfNodesAdded = leftFactor * (2 << D);
//		return nbOfNodesAdded;
//	}

	//POGING 2
	size_t factor_2 = 1;
	size_t factor_16 = 1;
	size_t nbOfNodesAdded = 1;
	if(depth >= nbOfQueuesOf2Nodes - 1)
	{
		factor_16 = static_cast<size_t>(pow(16.0, maxDepth - depth));
	}else
	{
		factor_16 = static_cast<size_t>(pow(16.0, nbOfQueuesOf16Nodes));
		factor_2 = static_cast<size_t>(pow(2.0, nbOfQueuesOf2Nodes - 1 - depth));
	}

	nbOfNodesAdded = factor_2 * factor_16;
	return nbOfNodesAdded;
}

/* SUPPOSES all queues are empty.
 Calculates the highest queue in which we can add a node, 
 so that the largest possible number of empty voxels is added,
 with that amount smaller than the number of empty voxels we want to add?
 */
int Tree4DBuilderDifferentSides_Time_longest::calculateQueueShouldItBePossibleToAddAllVoxelsAtOnce(const size_t nbOfEmptyVoxelsToAdd)
{
	//STEL: we hebben enkel queues van 16 nodes
	if(nbOfQueuesOf2Nodes == 0)
	{
		// In elke queue passen 16 nodes.
		// 1 node toevoegen --> 1 node toevoegen op maxDepth
		//  ...
		// 1 keer 16 nodes toevoegen  = 16^1 * 1 nodes toevoegen --> 1 node toevoegen op maxDepth - 1
		// 2 keer 16 nodes toevoegen  = 16^1 * 2 nodes toevoegen --> 2 nodes toevoegen op maxDepth - 1
		// ...
		// 16 keer 16 nodes toevoegen = 16^2 * 1 nodes toevoegen --> 1 node toevoegen op maxDepth - 2
		// 16^2 *1  + 1 nodes toevoegen --> 1 node toevoegen op maxDepths-2 , 1 node op maxDepth
		// ...
		// 16^a nodes toevoegen --> 1 node toevoegen op maxDepth - a

		// adding a node at maxDepth == adding 1 voxel				--> 16^0 voxels by adding at depth maxDepth - 0
		// adding a node at maxDepth - 1 == adding 16 voxels		--> 16^1 voxels by adding at depth maxDepth - 1
		//    (adding 2 nodes at maxDepth - 2 = adding 2 * 16 voxels)
		// adding a node at mxDepth - 2 = adding 16^2 voxels		--> 16^2 voxels by adding at depth maxDepth - 2
		// ...
		// adding a node at depth 0 = adding 16^maxDepth voxels		--> 16^maxDepth voxels by adding at depth maxDepth - maxDepth


		//  nbOfEmptyVoxelsToAdd = 16^a + remainder

		size_t a = findPowerOf16(nbOfEmptyVoxelsToAdd);

		size_t suggestedDepth = maxDepth - a;
		assert(suggestedDepth >= 0);
		assert(suggestedDepth <= maxDepth);

		return suggestedDepth;
	} else{ 
		/*
		gridsize_t = 2^y
		gridsize_s = 2^x  with y > x

		#vox = 2^(3x+y) ~> 2^(4x + D)    met D = y - x >= 0

		maxDepth = log2( max(gridsize_s, gridsize_t) ) ~> log2(gridsize_t) = y
		totalNbOfQueues = maxDepth + 1  ~> y + 1

		nbOfQueuesOf16Nodes = log2(gridsize_s) = x
		nbOfQueuesOf2Nodes = totalNbOfQueues - nbOfQueuesOf16Nodes ~> y + 1 - x = D + 1
		*/

		// we hebben x     queues van 16 nodes
		//			 D + 1 queues van  2 nodes
		// 
		// 16^x nodes toevoegen  
		//		= 2^4x nodes toevoegen
		//			--> 1 node toevoegen op maxDepth - x = y - x (onderste queue met 2 nodes)
		// ...
		// 2 * 16^x nodes toevoegen
		//		= 2^(4x + 1) nodes toevoegen
		//			--> 2 nodes toevoegen op maxDepth - x 
		//									= y- x (onderste queue met 2 nodes)
		//			--> 1 node toevoegen op maxDepth - x - 1 
		//									= y - x - 1
		// 4 * 16^x nodes toevoegen
		//		= 2^(4x + 2) nodes toevoegen
		//			-->  2 nodes toevoegen op maxDepth - x - 1
		//									= y - x - 1
		//			-->  1 node toevoegen op maxDepth - x - 2
		//									= y - x - 2
		// ... 
		// D * 16^x nodes toevoegen  = #vox
		//		= 2^(4x + D) nodes toevoegen
		//			--> 1 node toevoegen op maxDepth - x - D
		//									= y - x - D
		//									= 0   (bovenste queue)
		if(nbOfEmptyVoxelsToAdd < pow(gridsize_S, 4.0) * 2 ){
			// nbOfEmptyVoxelsToAdd < 2^(4x + 1)
			size_t a = findPowerOf16(nbOfEmptyVoxelsToAdd);

			size_t suggestedDepth = maxDepth - a; //y - a
			assert(suggestedDepth >= maxDepth - nbOfQueuesOf16Nodes);
			assert(suggestedDepth <= maxDepth);

			return suggestedDepth;
		}else{
			// nbOfEmptyVoxelsToAdd >= 2^(4x + 1) ( = 2 * 16^x)
			int factor = nbOfEmptyVoxelsToAdd / pow(gridsize_S, 4.0);
			int a = log2(factor); //a in {1, 2, ..., D}
			int suggestedDepth = maxDepth - nbOfQueuesOf16Nodes - a; //y - x - a
			assert(suggestedDepth >= 0);
			assert(suggestedDepth <= maxDepth - nbOfQueuesOf16Nodes - 1);

			return suggestedDepth;
		}	
	}
}

/*
Use this method to push back a node to the lowest queue
instead of pushing it to the last queue in queuesOfMax16

REASON: handling the special case where gridsize_t = 1
==> nbOfQueuesOf16Nodes == 0
==> queuesOfMax16.size() == 0
==>
	add the new leaf node to the lowest queue
	queuesOfMax16.at(nbOfQueuesOf16Nodes - 1).push_back(node); --> crash
*/
void Tree4DBuilderDifferentSides_Time_longest::push_backNodeToLowestQueue(Node4D& node)
{
	queuesOfMax16.at(nbOfQueuesOf16Nodes - 1).push_back(node);
}
