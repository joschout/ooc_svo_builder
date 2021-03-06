#include "Tree4DBuilder.h"
#include "morton4D.h"
#include <assert.h>
#include "octree_io.h"
#include "tree4d_io.h"

Tree4DBuilder::Tree4DBuilder(): 
	gridsize_S(1), gridsize_T(1),
	b_maxdepth(0),
	b_current_morton(0),
	b_max_morton(0), b_data_pos(0), b_node_pos(0), generate_levels(false),
	node_out_filepointer(nullptr), data_out_filepointer(nullptr)
{
}

void Tree4DBuilder::openOutputFiles(std::string base_filename)
{
	string nodes_name = base_filename + string(".tree4dnodes");
	string data_name = base_filename + string(".tree4ddata");
	node_out_filepointer = fopen(nodes_name.c_str(), "wb");
	data_out_filepointer = fopen(data_name.c_str(), "wb");
}

// Tree4DBuilder constructor: this initializes the builder and sets up the output files, ready to go
Tree4DBuilder::Tree4DBuilder(std::string base_filename, size_t gridsize_S, size_t gridsize_T, bool generate_levels) :
	gridsize_S(gridsize_S), gridsize_T(gridsize_T), b_node_pos(0),
	b_data_pos(0), b_current_morton(0),
	generate_levels(generate_levels), base_filename(base_filename)
{

	// Open output files
	openOutputFiles(base_filename);

	// Setup building variables
	b_maxdepth = log2((unsigned int)max(gridsize_S, gridsize_T));
	b_buffers.resize(b_maxdepth + 1);
	for (int i = 0; i < b_maxdepth + 1; i++) {
		b_buffers[i].reserve(16);
	}

	// Fill data arrays
	b_max_morton 
		= morton4D_Encode_for<uint64_t, uint_fast32_t>(
			(uint_fast32_t)gridsize_S - 1, (uint_fast32_t)gridsize_S - 1, (uint_fast32_t)gridsize_S - 1, (uint_fast32_t)gridsize_T - 1,
			(uint_fast32_t)gridsize_S, (uint_fast32_t)gridsize_S, (uint_fast32_t)gridsize_S, (uint_fast32_t)gridsize_T);

	writeVoxelData(data_out_filepointer, VoxelData(), b_data_pos); // first data point is NULL

#ifdef BINARY_VOXELIZATION
	VoxelData v = VoxelData(0, vec3(), vec3(1.0, 1.0, 1.0)); // We store a simple white voxel in case of Binary voxelization
	writeVoxelData(data_out_filepointer, v, b_data_pos); // all leafs will refer to this
#endif

}

// Finalize the tree: add rest of empty nodes, make sure root node is on top
void Tree4DBuilder::finalizeTree() {
	// fill octree
	if (b_current_morton < b_max_morton) {
		fastAddEmpty((b_max_morton - b_current_morton) + 1);
	}

	// write root node
	writeNode4D(node_out_filepointer, b_buffers[0][0], b_node_pos);

	// write header
	Tree4DInfo tree4D_info(1, base_filename, gridsize_S, gridsize_T, b_node_pos, b_data_pos);

	//svo_algorithm_timer.stop(); svo_io_output_timer.start(); // TIMING
	writeOctreeHeader(base_filename + string(".tree4d"), tree4D_info);
	//svo_io_output_timer.stop(); svo_algorithm_timer.start(); // TIMING

	// close files
	fclose(data_out_filepointer);
	fclose(node_out_filepointer);
}

// Group 16 nodes, write non-empty nodes to disk and create parent node
Node4D Tree4DBuilder::groupNodes(const vector<Node4D> &buffer) {
	Node4D parent = Node4D();
	bool first_stored_child = true;
	for (int k = 0; k < 16; k++) {
		if (!buffer[k].isNull()) {
			if (first_stored_child) {
				//svo_algorithm_timer.stop(); svo_io_output_timer.start(); // TIMING
				parent.children_base = writeNode4D(node_out_filepointer, buffer[k], b_node_pos);
				//svo_io_output_timer.stop(); svo_algorithm_timer.start(); // TIMING
				parent.children_offset[k] = 0;
				first_stored_child = false;
			}
			else {
				//svo_algorithm_timer.stop(); svo_io_output_timer.start(); // TIMING
				parent.children_offset[k] = (char)(writeNode4D(node_out_filepointer, buffer[k], b_node_pos) - parent.children_base);
				//svo_io_output_timer.stop(); svo_algorithm_timer.start(); // TIMING
			}
		}
		else {
			parent.children_offset[k] = NOCHILD;
		}
	}

	// SIMPLE LEVEL CONSTRUCTION
	if (generate_levels) {
		VoxelData d = VoxelData();
		float notnull = 0.0f;
		for (int i = 0; i < 16; i++) { // this node has no data: need to refine
			if (!buffer[i].isNull())
				notnull++;
			d.color += buffer[i].data_cache.color;
			d.normal += buffer[i].data_cache.normal;
		}
		d.color = d.color / notnull;
		vec3 tonormalize = (vec3)(d.normal / notnull);
		d.normal = normalize(tonormalize);
		// set it in the parent node
		//svo_algorithm_timer.stop(); svo_io_output_timer.start(); // TIMING
		parent.data = writeVoxelData(data_out_filepointer, d, b_data_pos);
		//svo_io_output_timer.stop(); svo_algorithm_timer.start(); // TIMING
		parent.data_cache = d;
	}

	return parent;
}

// Add an empty datapoint at a certain buffer level, and refine upwards from there
void Tree4DBuilder::addEmptyVoxel(const int buffer) {
	b_buffers[buffer].push_back(Node4D());
	refineBuffers(buffer);
	b_current_morton = (uint64_t)(b_current_morton + pow(16.0, b_maxdepth - buffer)); // because we're adding at a certain level
}

// REFINE BUFFERS: check all levels from start_depth up and group 16 nodes on a higher level
void Tree4DBuilder::refineBuffers(const int start_depth) {
	for (int d = start_depth; d >= 0; d--) {
		if (b_buffers[d].size() == 16) { // if we have 16 nodes
			assert(d - 1 >= 0);
			if (isBufferEmpty(b_buffers[d])) {
				b_buffers[d - 1].push_back(Node4D()); // push back NULL node to represent 16 empty nodes
			}
			else {
				b_buffers[d - 1].push_back(groupNodes(b_buffers[d])); // push back parent node
			}
			b_buffers.at(d).clear(); // clear the 16 nodes on this level
		}
		else {
			break; // break the for loop: no upper levels will need changing
		}
	}
}

// Add a datapoint to the octree: this is the main method used to push datapoints
void Tree4DBuilder::addVoxel(const uint64_t morton_number) {
	// Padding for missed morton numbers
	if (morton_number != b_current_morton) {
		fastAddEmpty(morton_number - b_current_morton);
	}

	// Create node
	Node4D node = Node4D(); // create empty node
	node.data = 1; // all nodes in binary voxelization refer to this
				   // Add to buffer

	b_buffers.at(b_maxdepth).push_back(node);
	// Refine buffers
	refineBuffers(b_maxdepth);

	b_current_morton++;
}

// Add a datapoint to the octree: this is the main method used to push datapoints
void Tree4DBuilder::addVoxel(const VoxelData& data) {
	// Padding for missed morton numbers
	if (data.morton != b_current_morton) {
		fastAddEmpty(data.morton - b_current_morton);
	}

	// Create node
	Node4D node = Node4D(); // create empty node
						// Write data point
						//svo_algorithm_timer.stop(); svo_io_output_timer.start(); // TIMING
	node.data = writeVoxelData(data_out_filepointer, data, b_data_pos); // store data
															//svo_io_output_timer.stop(); svo_algorithm_timer.start(); // TIMING
	node.data_cache = data; // store data as cache
							// Add to buffers
	b_buffers.at(b_maxdepth).push_back(node);
	// Refine buffers
	refineBuffers(b_maxdepth);

	b_current_morton++;
}