#pragma warning(disable : 4503)
#include "voronoi.hpp"
#include "map"

VoronoiDiagram::VoronoiDiagram() {

}

void VoronoiDiagram::AddPoint(Point p) {
	points.push_back(p);
}

void VoronoiDiagram::AddSegment(Segment s) {
	segments.push_back(s);
}

void VoronoiDiagram::Construct() {
	construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &vd);
}

void VoronoiDiagram::GetEdges(std::vector<c_Vertex> &vertices, std::vector<c_Edge> &edges) {
	std::set<const voronoi_edge<double> *> visited;
	std::map<const voronoi_vertex<double> *, long long> vertexMap;

	for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); 	it != vd.edges().end(); ++it) {
		if (visited.find(&(*it)) != visited.end())
			continue;

		const voronoi_vertex<double> *start = it->vertex0();
		const voronoi_vertex<double> *end = it->vertex1();

		long long startIndex = -1;
		if (start != 0) {
			std::map<const voronoi_vertex<double> *, long long>::iterator it = vertexMap.find(start);

			if (it == vertexMap.end())
			{
				c_Vertex startVertex = c_Vertex(start->x(), start->y());
				startIndex = (long long) vertices.size();
				vertices.push_back(startVertex);
				vertexMap[start] = startIndex;
			}
			else {
				startIndex = it->second;
			}
		}

		long long endIndex = -1;
		if (end != 0) {
			std::map<const voronoi_vertex<double> *, long long>::iterator it = vertexMap.find(end);

			if (it == vertexMap.end())
			{
				c_Vertex endVertex = c_Vertex(end->x(), end->y());
				endIndex = (long long) vertices.size();
				vertices.push_back(endVertex);
				vertexMap[end] = endIndex;
			}
			else {
				endIndex = it->second;
			}
		}

		size_t firstIndex = it->cell()->source_index();

		const voronoi_edge<double> *twin = it->twin();
		visited.insert(twin);

		size_t secondIndex = twin->cell()->source_index();

		edges.push_back(c_Edge(startIndex, endIndex, it->is_primary(), firstIndex, secondIndex, it->is_linear()));
	}
}

void VoronoiDiagram::GetCells(std::vector<c_Vertex> &vertices, std::vector<c_Edge> &edges, std::vector<c_Cell> &cells) {
	std::map<const voronoi_vertex<double> *, long long> vertexMap;
	std::map<const voronoi_edge<double> *, long long> edgeMap;

    //An identifier for cells
    long long cell_identifier = 0;

	for (voronoi_diagram<double>::const_cell_iterator itcell = vd.cells().begin();
			itcell != vd.cells().end();
			++itcell) {

		//Identify the source type
		int source_category = -1;
		if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT){
			source_category = 0;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT){
			source_category = 1;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT){
			source_category = 2;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_INITIAL_SEGMENT){
			source_category = 3;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_REVERSE_SEGMENT){
			source_category = 4;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_GEOMETRY_SHIFT){
			source_category = 5;
		}
		else if (itcell->source_category() == boost::polygon::SOURCE_CATEGORY_BITMASK){
			source_category = 6;
		}

		if(!itcell->is_degenerate()){
			c_Cell cell = c_Cell(cell_identifier, itcell->source_index(), itcell->contains_point(), itcell->contains_segment(), false, source_category);
			const voronoi_diagram<double>::edge_type *edge = itcell->incident_edge();
			if(edge != NULL){
				do {
					const voronoi_vertex<double> *start = edge->vertex0();
					const voronoi_vertex<double> *end = edge->vertex1();

					//Add an map the vertices
					long long startIndex = -1;
					if(start != 0){
						std::map<const voronoi_vertex<double> *, long long>::iterator vertexMapIterator = vertexMap.find(start);
						if(vertexMapIterator == vertexMap.end()){
							c_Vertex endVertex = c_Vertex(start->x(), start->y());
							startIndex = (long long) vertices.size();
							vertices.push_back(endVertex);
							vertexMap[start] = startIndex;
						}
						else {
							startIndex = vertexMapIterator->second;
						}
					}

					long long endIndex = -1;
					if(end != 0){
						std::map<const voronoi_vertex<double> *, long long>::iterator vertexMapIterator = vertexMap.find(end);
						if(vertexMapIterator == vertexMap.end()){
							c_Vertex endVertex = c_Vertex(end->x(), end->y());
							endIndex = (long long) vertices.size();
							vertices.push_back(endVertex);
							vertexMap[end] = endIndex;
						}
						else {
							endIndex = vertexMapIterator->second;
						}
					}

					if(startIndex == -1 || endIndex == -1){
						cell.is_open = true;
					}

					cell.vertices.push_back(startIndex);

					//Add and map the edge
					c_Edge outputEdge = c_Edge(startIndex, endIndex, edge->is_primary(), edge->cell()->source_index(), edge->twin()->cell()->source_index(), edge->is_linear(), cell_identifier, -1);
					size_t edge_index = edges.size();
					edgeMap[edge] = edge_index;
					edges.push_back(outputEdge);
					cell.edges.push_back(edge_index);

					edge = edge->next();
				} while (edge != itcell->incident_edge() && edge != NULL);
			}
			cells.push_back(cell);
			cell_identifier++;
		}
	}

    //Second iteration for twins
    //This part can probably optimized - TBD
    for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it) {
    	const voronoi_diagram<double>::cell_type &cell = *it;

    	//Don't do anything if the cells is degenerate
    	if (!cell.is_degenerate()){
    		//Iterate throught the edges
    		const voronoi_diagram<double>::edge_type *edge = cell.incident_edge();
    		if (edge != NULL)
    		{
    			do {
    				long long edge_id = -1;
    				std::map<const voronoi_diagram<double>::edge_type *, long long>::iterator edgeMapIterator = edgeMap.find(edge);
    				if (edgeMapIterator != edgeMap.end()){
    					edge_id = edgeMapIterator->second;
    				}

    				if (edge_id != -1){
    					edgeMapIterator = edgeMap.find(edge->twin());
    					if (edgeMapIterator != edgeMap.end()){
    						edges[edge_id].twin = edgeMapIterator->second;
    					}
    				}
    				//Move to the next edge
    				edge = edge->next();
    			} while (edge != cell.incident_edge());
    		}
    	}
    }
}

std::vector<Point> VoronoiDiagram::GetPoints() {
	return points;
}

std::vector<Segment> VoronoiDiagram::GetSegments() {
	return segments;
}


long long VoronoiDiagram::CountVertices(){
	return vd.num_vertices();
}

long long VoronoiDiagram::CountEdges(){
	return vd.num_edges();
}

long long VoronoiDiagram::CountCells(){
	return vd.num_cells();
}


void VoronoiDiagram::MapVertexIndexes(){
		long long index = 0;
		for (voronoi_diagram<double>::const_vertex_iterator it = vd.vertices().begin(); it != vd.vertices().end(); ++it) {
			const voronoi_diagram<double>::vertex_type* vertex = &(*it);
			map_indexes_to_vertices.insert(index_to_vertex(index, vertex));
			map_vertices_to_indexes.insert(vertex_to_index(vertex, index));
			index++;
		}
}

void VoronoiDiagram::MapEdgeIndexes(){
	long long index = 0;
	for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it) {
		const voronoi_diagram<double>::edge_type* edge = &(*it);
		map_indexes_to_edges.insert(index_to_edge(index, edge));
		map_edges_to_indexes.insert(edge_to_index(edge, index));
		index++;
	}
}

void VoronoiDiagram::MapCellIndexes(){
	long long index = 0;
	for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it) {
		const voronoi_diagram<double>::cell_type* cell = &(*it);
		map_indexes_to_cells.insert(index_to_cell(index, cell));
		map_cells_to_indexes.insert(cell_to_index(cell, index));
		index++;
	}
}

c_Vertex VoronoiDiagram::GetVertex(long long index){
	const voronoi_diagram<double>::vertex_type* vertex = map_indexes_to_vertices[index];
	return c_Vertex(vertex->x(), vertex->y());
}

c_Edge VoronoiDiagram::GetEdge(long long index)
{
	const voronoi_diagram<double>::edge_type* edge = map_indexes_to_edges[index];

	//Find vertex references
	const voronoi_diagram<double>::vertex_type * start = edge->vertex0();
	const voronoi_diagram<double>::vertex_type * end = edge->vertex1();

	long long start_id = map_vertices_to_indexes[start];
	long long end_id = map_vertices_to_indexes[end];

	//Find the twin reference using the segment object
	const voronoi_diagram<double>::edge_type * twin = edge->twin();
	long long twinIndex = -1;
	if (edge != NULL){
		twinIndex = map_edges_to_indexes[twin];
	}

	//Find the cell reference using ther cell object
	long long cellIndex = map_cells_to_indexes[edge->cell()];

	//Return the object
	return c_Edge(
		start_id,
		end_id,
		edge->is_primary(),
		-1,
		-1,
		edge->is_linear(),
		cellIndex,
		twinIndex
	);
}

c_Cell VoronoiDiagram::GetCell(long long index)
{
	//std::map<long long, const voronoi_diagram<double>::cell_type *>::iterator cellMapIterator = cellMap2.find(index);
	const voronoi_diagram<double>::cell_type* cell = map_indexes_to_cells[index];
	std::vector<long long> edge_identifiers;
	std::vector<long long> vertex_identifiers;

	bool is_open = false;

	//Identify the source type
	int source_category = -1;
	if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT){
		source_category = 0;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT){
		source_category = 1;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT){
		source_category = 2;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_INITIAL_SEGMENT){
		source_category = 3;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_REVERSE_SEGMENT){
		source_category = 4;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_GEOMETRY_SHIFT){
		source_category = 5;
	}
	else if (cell->source_category() == boost::polygon::SOURCE_CATEGORY_BITMASK){
		source_category = 6;
	}


	const voronoi_diagram<double>::edge_type* edge = cell->incident_edge();
	if (edge != NULL)
	{
		do {
			//Get the edge index
			long long edge_index = map_edges_to_indexes[edge];
			edge_identifiers.push_back(edge_index);

			if (edge->vertex0() == NULL || edge->vertex1() == NULL)
				is_open = true;

			long long edge_start = -1;
			long long edge_end = -1;

			if (edge->vertex0() == NULL){
				edge_start = map_vertices_to_indexes[edge->vertex0()];
			}

			if (edge->vertex1() == NULL){
				edge_end = map_vertices_to_indexes[edge->vertex1()];
			}

			long vertices_count = vertex_identifiers.size();
			if (vertices_count == 0){
				vertex_identifiers.push_back(edge_start);
			}
			else{
				if (vertex_identifiers[vertices_count - 1] != edge_start){
					vertex_identifiers.push_back(edge_start);
				}
			}
			vertex_identifiers.push_back(edge_end);
			//Move to the next edge
			edge = edge->next();

		} while (edge != cell->incident_edge());
	}

	c_Cell c_cell = c_Cell(
		index,
		cell->source_index(),
		cell->contains_point(),
		cell->contains_segment(),
		is_open,
		source_category
	);

	c_cell.vertices = vertex_identifiers;
	c_cell.edges = edge_identifiers;

	return c_cell;
}
