#include "config.h"
#include "bone_geometry.h"
#include "texture_to_render.h"
#include <fstream>
#include <queue>
#include <iostream>
#include <stdexcept>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <math.h>

#define PI 3.14159265

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	size_t count = std::min(v.size(), static_cast<size_t>(10));
	for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
	os << "size = " << v.size() << "\n";
	return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds)
{
	os << "min = " << bounds.min << " max = " << bounds.max;
	return os;
}

glm::fquat splineQuat(glm::fquat q0, glm::fquat q1, glm::fquat q2, glm::fquat q3, float tau);
glm::fquat splineQuat(glm::fquat q0, glm::fquat q1, glm::fquat q2, glm::fquat q3, float tau) {
	glm::fquat s1 = q1 * glm::exp( (glm::log( glm::inverse(q1) * q2) + glm::log( glm::inverse(q1) * q0 ) )/(-4.0f));
	glm::fquat s2 = q2 * glm::exp( (glm::log( glm::inverse(q2) * q3) + glm::log( glm::inverse(q2) * q1 ) )/(-4.0f));
	glm::fquat temp_1 = glm::mix(q1, q2, tau);
	glm::fquat temp_2 = glm::mix(s1, s2, tau);
	glm::fquat result = glm::mix(temp_1, temp_2, 2.0f*tau*(1.0f-tau));
	return result;
}



void Skeleton::refreshCache(Configuration* target)
{
	if (target == nullptr)
		target = &cache;
	target->rot.resize(joints.size());
	target->trans.resize(joints.size());
	bone_transforms.resize(bones.size());
	for (size_t i = 0; i < bones.size(); i++) {
		if (bones[i] != nullptr){
			bone_transforms[i] = bones[i]->getCylinderTransform();
			joints[bones[i]->parent_index].orientation = bones[i]->orientation;
			joints[bones[i]->parent_index].rel_orientation = bones[i]->rel_orientation;
		}
	}

	for (size_t i = 0; i < joints.size(); i++) {
		if (joints[i].parent_index != -1){
			joints[i].position = glm::vec3(bones[i]->deformed_transform * glm::inverse(bones[i]->undeformed_transform) * glm::vec4(joints[i].init_position,1.0));
		} else {
			int root_child = joints[i].children[0];
			joints[i].position = glm::vec3(bones[root_child]->translation[3][0], bones[root_child]->translation[3][1],bones[root_child]->translation[3][2]);
		}
		
	}

	for (size_t i = 0; i < joints.size(); i++) {
		target->rot[i] = joints[i].orientation;
		target->trans[i] = joints[i].position;
	}
}

const glm::vec3* Skeleton::collectJointTrans() const
{
	return cache.trans.data();
}

const glm::fquat* Skeleton::collectJointRot() const
{
	return cache.rot.data();
}

const glm::mat4* Skeleton::collectBoneTransforms() const
{
	return bone_transforms.data();
}

const glm::mat4* Skeleton::getBoneTransform(int bone_id) const
{
	return &bone_transforms[bone_id];;
}

// FIXME: Implement bone animation.

void Skeleton::doTranslation(glm::vec3 diff_translation, int root_id){
	Joint root = joints[root_id];
	for (size_t i = 0; i < root.children.size(); i++){
		bones[root.children[i]]->performTranslation(diff_translation);
	}
}

void Skeleton::doAnimationTranslation(glm::vec3 diff_translation, int root_id){
	Joint root = joints[root_id];
	for (size_t i = 0; i < root.children.size(); i++){
		bones[root.children[i]]->performAnimationTranslation(diff_translation);
	}
}



Bone* Skeleton::getBone(int bone_id)
{
	return bones[bone_id];
}


void Skeleton::constructBone(int joint_id)
{
	if (joint_id <= 0 || bones[joint_id] != nullptr){
		return;
	}


	Joint end_j = joints[joint_id];
	if (end_j.parent_index == -1){
		bones[joint_id] = nullptr;
		return;
	}
	Joint start_j = joints[end_j.parent_index];
	Bone *bone = new Bone(start_j, end_j);
	joints[end_j.parent_index].children.emplace_back(end_j.joint_index);
	bones[joint_id] = bone;
	glm::vec3 diff;
	if (start_j.parent_index >= 0){
		bone->parent = bones[end_j.parent_index];
		diff = start_j.position - joints[start_j.parent_index].position;
	} else {
		bone->parent = nullptr;
		diff = start_j.position;
	}
	bone->translation = glm::mat4(1.0f);
	bone->translation[3][0] = diff.x;
	bone->translation[3][1] = diff.y;
	bone->translation[3][2] = diff.z;
	bone->init_translation = bone->translation;
	if (start_j.parent_index == -1){
		bone->undeformed_transform = bone->translation;
		bone->deformed_transform = bone->translation * glm::toMat4(glm::normalize(bone->rel_orientation));
		bone->root_joint_index = start_j.joint_index;
	} else {
		bone->undeformed_transform = (bone->parent)->undeformed_transform * bone->translation;
		bone->deformed_transform = (bone->parent)->deformed_transform * bone->translation * glm::toMat4(glm::normalize(bone->rel_orientation));
		bone->root_joint_index = bone->parent->root_joint_index;

	}
	if (bone->parent != nullptr) {
		bone->parent->children.emplace_back(bone);
	}
		
}

void Edge::calculateNormal() {
	glm::vec2 diff = vertex_2->position - vertex_2->position;
	glm::vec2 nor = glm::vec2(-diff[1], diff[0]);
	normal = nor;
}

void Edge::initiateNormal() {
	glm::vec2 diff = vertex_2->position_init - vertex_2->position_init;
	glm::vec2 nor = glm::vec2(-diff[1], diff[0]);
	normal = nor;
	normal_init = nor;
}

void Cage::updateNormals() {
	for (auto& edge : edges) {
		edge.calculateNormal();
	}
}

void Cage::initNormals() {
	for (auto& edge : edges) {
		edge.initiateNormal();
	}
}

void Cage::initWeights(glm::vec2 eta) {
	//initialize phi and varphi functions, as well as scalings
	Weights weights;
	for (uint i = 0; i < vertices.size(); i++) {
		weights.weight_vertices.emplace_back(0.0f);
	}
	for (uint i = 0; i < edges.size(); i++) {
		weights.weight_edges.emplace_back(0.0f);
	}

	for (uint i = 0; i < edges.size(); i++) {
		Edge edge = edges[(int)i];
		Vertex* v1 = edge.vertex_1;
		Vertex* v2 = edge.vertex_2;
		glm::vec2 a = v2->position_init - v1->position_init;
		glm::vec2 b = v1->position_init - eta;
		float Q = glm::dot(a,a);
		float S = glm::dot(b,b);
		float R = 2 * glm::dot(a,b);
		float BA = glm::length(a) * glm::dot(b,edge.normal_init);
		float SRT = sqrt(4*S*Q - R*R);
		float L0 = log(S);
		float L1 = log(S+Q+R);
		float A0 = atan(R/SRT) / SRT;
		float A1 = atan((2*Q+R)/SRT) / SRT;
		float A10 = A1 - A0;
		float L10 = L1 - L0;
		weight_edges[i] = -glm::length(a) / (4*PI) * ((4*S-R*R/Q)*A10 + R/(2*Q)*L10 + L1 -2);
		weight_vertices[v2.id] = weight_vertices[v2.id] - BA/(2*PI) * (L10/(2*Q) - A10 *R/Q);
		weight_vertices[v1.id] = weight_vertices[v1.id] + BA/(2*PI) * (L10/(2*Q) - A10 *(2+R/Q));
	}

	weigtht_functions.emplace_back(weights);

}

void Cage::updateWeights() {
	for (uint i=0; i<edges.size(); i++) {
		Edge edge = edges[(int)i];
		Vertex* v1 = edge.vertex_1;
		Vertex* v2 = edge.vertex_2;
		edge_scalings[(int)i] = glm::length(v2->position - v1->position) / glm::length(v2->position_init - v1->position_init);
	}

}


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::loadPmd(const std::string& fn)
{
	MMDReader mr;
	mr.open(fn);
	mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
	computeBounds();
	mr.getMaterial(materials);

	// FIXME: load skeleton and blend weights from PMD file,
	//        initialize std::vectors for the vertex attributes,
	//        also initialize the skeleton as needed
	int id = 0;
	int parent = 0;
	glm::vec3 wcoord;

	while (mr.getJoint(id, wcoord, parent)){
		Joint joint = Joint(id, wcoord, parent);
		joint.position.z = 0.0;
		skeleton.joints.emplace_back(joint);
		id++;
	}

	skeleton.bones.resize(skeleton.joints.size());
	for (uint i = 0 ; i < skeleton.joints.size(); i++){
		if (skeleton.joints[i].parent_index == -1){
			skeleton.bones[i] = nullptr;
		}
	}
	//skeleton.bones[0] = nullptr;

	for (uint i = 1; i < skeleton.joints.size(); i++){
		skeleton.constructBone(i);
	}


	std::vector<SparseTuple> weights;
	mr.getJointWeights(weights);

	for (SparseTuple& sparse_tuple : weights) {
		int v_id = sparse_tuple.vid;
		joint0.emplace_back(sparse_tuple.jid0);
		joint1.emplace_back(sparse_tuple.jid1);
		weight_for_joint0.emplace_back(sparse_tuple.weight0);
		vector_from_joint0.emplace_back(glm::vec3(vertices[v_id]) - skeleton.joints[sparse_tuple.jid0].position);
		vector_from_joint1.emplace_back(glm::vec3(vertices[v_id]) - skeleton.joints[sparse_tuple.jid1].position);

	}
}

void Mesh::generateCage() {
	cage.getNormals();
}

void Mesh::initCageWeights() {
	for (auto& joint : skeleton.joints) {
		glm::vec2 pos = glm::vec2(joint.position[0], joint.position[1]);
		cage.initWeights(pos);
	}

	for (int i=0; i < cage.edges.size(); i++) {
		cage.edge_scalings.emplace_back(0.0f);
	}
}

void Mesh::updateCageWeights() {
	cage.updateWeights();
}

int Mesh::getNumberOfBones() const
{
	return skeleton.joints.size();
}

void Mesh::computeBounds()
{
	bounds.min = glm::vec3(std::numeric_limits<float>::max());
	bounds.max = glm::vec3(-std::numeric_limits<float>::max());
	for (const auto& vert : vertices) {
		bounds.min = glm::min(glm::vec3(vert), bounds.min);
		bounds.max = glm::max(glm::vec3(vert), bounds.max);
	}
}

void Mesh::updateAnimation(float t)
{
	skeleton.refreshCache(&currentQ_);
	// FIXME: Support Animation Here
	int frame_id = floor(t);
	if (t != -1.0 && frame_id + 1 < (int)key_frames.size()){
		float tao = t - frame_id;
		KeyFrame frame;
		if (isSpline()) {
			KeyFrame::interpolate_spline(key_frames, t, frame);
			KeyFrame::linear_trans(key_frames[frame_id], key_frames[frame_id + 1], tao, frame);
		} else {
			KeyFrame::interpolate(key_frames[frame_id], key_frames[frame_id + 1], tao, frame);
		}
		
		updateSkeleton(frame);
	}

	//skeleton.refreshCache(&currentQ_);
}

void Mesh::updateSkeleton(KeyFrame key_frame){
	for (uint i = 0; i < skeleton.joints.size(); i++){
		skeleton.joints[i].rel_orientation = key_frame.rel_rot[i];
	}

	for (uint i = 0; i < skeleton.joints.size(); i++){
		if (skeleton.joints[i].parent_index == -1){
			skeleton.doAnimationTranslation(key_frame.root_trans, skeleton.joints[i].joint_index);
			skeleton.joints[i].orientation = skeleton.joints[i].rel_orientation;
			updateFromRel(skeleton.joints[i]);
		}
	}
}

void Mesh::updateFromRel(Joint& parent){
	for (int child_id : parent.children){
		Joint& child = skeleton.joints[child_id];
		child.orientation = child.rel_orientation * parent.orientation;
		//child.position = glm::vec3(skeleton.bones[child.joint_index]->deformed_transform * glm::inverse(skeleton.bones[child.joint_index]->undeformed_transform) * glm::vec4(child.init_position,1.0));
		Bone* bone = skeleton.bones[child_id];
		bone->rel_orientation = parent.rel_orientation;
		bone->orientation = parent.orientation;
		if (parent.parent_index == -1){
			bone->deformed_transform =  bone->translation * glm::toMat4(bone->rel_orientation);
		} else {
			bone->deformed_transform = bone->parent->deformed_transform * bone->translation * glm::toMat4(bone->rel_orientation);
		}

		updateFromRel(child);
		
	}
}

Bone* Mesh::getBone(int bone_id){
	return skeleton.getBone(bone_id);
}



const Configuration*
Mesh::getCurrentQ() const
{
	return &currentQ_;
}



glm::mat4 Bone::getTranslation(){
	return translation;
}

glm::mat4 Bone::getUndeformed(){
	return undeformed_transform;
}

glm::mat4 Bone::getDeformed(){
	return deformed_transform;
}

glm::fquat Bone::getOrientation(){
	return orientation;
}

glm::fquat Bone::getRelOrientation(){
	return rel_orientation;
}


glm::vec4 Bone::boneAlign(glm::vec4 bone_coor){
	return alignMatrix * bone_coor;
}

glm::mat4 Bone::getCylinderTransform(){
	glm::mat4 trans = deformed_transform * inverseAlignMatrix;
	glm::mat4 scale = glm::mat4(1.0f);
	scale[0][0] = length;
	return trans * scale;
}

void Bone::performRotate(glm::fquat rotate_quat){
	glm::mat4 previous_rel = glm::toMat4(rel_orientation);
	glm::mat4 previous_ori = glm::toMat4(orientation);
	glm::mat4 update_rel = glm::toMat4(rotate_quat) * previous_rel;
	glm::mat4 update_ori = glm::toMat4(rotate_quat) * previous_ori;
	rel_orientation = glm::normalize(glm::toQuat(update_rel));
	orientation = glm::normalize(glm::toQuat(update_ori));

	deformed_transform = deformed_transform * glm::inverse(previous_rel) * update_rel;

	for (size_t i=0; i < children.size(); i++){
		children[i]->performParentRotate();
	}
}

void Bone::performRotateTotal(glm::fquat rotate_quat){
	if (parent != nullptr){
		for (size_t i = 0; i < parent->children.size(); i++){
			Bone* child = parent->children[i];
			child->performRotate(rotate_quat);
		}
	} else {
		performRotate(rotate_quat);
	}
}


void Bone::performParentRotate() {
	deformed_transform = parent->deformed_transform * translation * glm::toMat4(rel_orientation);
	glm::mat4 update_ori = glm::toMat4(parent->orientation) * glm::toMat4(rel_orientation);
	orientation = glm::normalize(glm::toQuat(update_ori));
	for (size_t i=0; i < children.size(); i++){
		children[i]->performParentRotate();
	}
}

void Bone::performTranslation(glm::vec3 diff_translation){
	translation[3][0] += 20.0f * diff_translation.x;
	translation[3][1] += 20.0f * diff_translation.y;
	translation[3][2] += 20.0f * diff_translation.z;
	deformed_transform = translation * glm::toMat4(glm::normalize(rel_orientation));
	for (size_t i=0; i < children.size(); i++){
		children[i]->performParentTranslation();
	}
}

void Bone::performParentTranslation() {
	deformed_transform = parent->deformed_transform * translation * glm::toMat4(glm::normalize(rel_orientation));
	for (size_t i=0; i < children.size(); i++){
		children[i]->performParentTranslation();
	}
}

void Bone::performAnimationTranslation(glm::vec3 diff_translation){
	translation = init_translation;
	translation[3][0] +=  diff_translation.x;
	translation[3][1] +=  diff_translation.y;
	translation[3][2] +=  diff_translation.z;
	deformed_transform = translation * glm::toMat4(glm::normalize(rel_orientation));
	for (size_t i=0; i < children.size(); i++){
		children[i]->performParentTranslation();
	}
}



void KeyFrame::interpolate(const KeyFrame& from,
                        const KeyFrame& to,
                        float tau,
                        KeyFrame& target) {
	std::vector<glm::fquat> rel_rot_from = from.rel_rot;
	std::vector<glm::fquat> rel_rot_to = to.rel_rot;
	target.rel_rot.resize(rel_rot_from.size());
	for (uint i = 0; i < target.rel_rot.size(); i++){
		target.rel_rot[i] = glm::fastMix(rel_rot_from[i], rel_rot_to[i], tau);
	}
	target.root_trans = (1 - tau) * from.root_trans + tau * to.root_trans;
}

void KeyFrame::interpolate_spline(const std::vector<KeyFrame>& key_frames, float t, KeyFrame& target) {
	//interpolate quartenioons with spherical spline interpolation, root_trans with natural cubic spline
	for (uint i = 0; i < key_frames[0].rel_rot.size(); i++) {
		std::vector<glm::fquat> bone_rels;
		for (uint f_id = 0; f_id < key_frames.size(); f_id++) {
			bone_rels.emplace_back(key_frames[f_id].rel_rot[i]);
		}
		glm::fquat spline_quat = calculateSplineQuat(bone_rels, t);
		target.rel_rot.emplace_back(spline_quat);
	}
}

void KeyFrame::linear_trans(const KeyFrame& from,
                        const KeyFrame& to,
                        float tau,
                        KeyFrame& target) {
	target.root_trans = (1 - tau) * from.root_trans + tau * to.root_trans;
}

glm::fquat KeyFrame::calculateSplineQuat(const std::vector<glm::fquat>& bone_rels, float t) {
	int id_0 = glm::clamp<int>(t-1, 0, bone_rels.size()-1);
	int id_1 = glm::clamp<int>(t, 0, bone_rels.size()-1);
	int id_2 = glm::clamp<int>(t+1, 0, bone_rels.size()-1);
	int id_3 = glm::clamp<int>(t+2, 0, bone_rels.size()-1);
	float tau = t - floor(t);
	return splineQuat(bone_rels[id_0], bone_rels[id_1], bone_rels[id_2], bone_rels[id_3], tau);
	//return glm::fquat(1.0,0.0,0.0,0.0);
}


void Mesh::createKeyFrame(){
	KeyFrame frame;
	for (uint i = 0; i < skeleton.joints.size(); i++){
		frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);
	}
	frame.root_trans = skeleton.joints[0].position - skeleton.joints[0].init_position;
	key_frames.emplace_back(frame);
	if (key_frames.size() == 1){
		for (uint i = 1; i < skeleton.bones.size(); i++){
			skeleton.bones[i]->init_translation = skeleton.bones[i]->translation;
		}
	}
}


void Mesh::deleteKeyFrame(int frame_id){
	key_frames.erase(key_frames.begin() + frame_id);
}

void Mesh::updateKeyFrame(int frame_id){
	if (frame_id < 0){
		return;
	}
	KeyFrame frame;
	for (uint i = 0; i < skeleton.joints.size(); i++){
		frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);
	}
	frame.root_trans = skeleton.joints[0].position - skeleton.joints[0].init_position;
	key_frames[frame_id] = frame;
}

void Mesh::spaceKeyFrame(int frame_id) {
	updateSkeleton(key_frames[frame_id]);
	skeleton.refreshCache(&currentQ_);
}

void Mesh::insertKeyFrame(int frame_id) {
	KeyFrame frame;
	for (uint i = 0; i < skeleton.joints.size(); i++){
		frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);
	}
	frame.root_trans = skeleton.joints[0].position - skeleton.joints[0].init_position;
	std::vector<KeyFrame>::iterator iterator = key_frames.begin();
	key_frames.insert(iterator+frame_id, frame);
	if (frame_id == 0){
		for (uint i = 1; i < skeleton.bones.size(); i++){
			skeleton.bones[i]->init_translation = skeleton.bones[i]->translation;
		}
	}
}

// glm::fquat splineQuat(glm::fquat q0, glm::fquat q1, glm::fquat q2, glm::fquat q3, float tau) {
// 	glm::fquat s1 = q1 * glm::exp( (glm::log( glm::inverse(q1) * q2) + glm::log( glm::inverse(q1) * q0 ) )/(-4.0f));
// 	glm::fquat s2 = q2 * glm::exp( (glm::log( glm::inverse(q2) * q3) + glm::log( glm::inverse(q2) * q1 ) )/(-4.0f));
// 	glm::fquat temp_1 = glm::mix(q1, q2, tau);
// 	glm::fquat temp_2 = glm::mix(s1, s2, tau);
// 	glm::fquat result = glm::mix(temp_1, temp_2, 2*tau*(1.0-tau));
// 	return result;
// }