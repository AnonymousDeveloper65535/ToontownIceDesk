// Filename: eggJointNodePointer.cxx
// Created by:  drose (26Feb01)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "eggJointNodePointer.h"

#include "dcast.h"
#include "eggObject.h"
#include "eggGroup.h"
#include "pointerTo.h"


TypeHandle EggJointNodePointer::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
EggJointNodePointer::
EggJointNodePointer(EggObject *object) {
  _joint = DCAST(EggGroup, object);

  if (_joint != (EggGroup *)NULL && _joint->is_joint()) {
    // Quietly insist that the joint has a transform, for neatness.  If
    // it does not, give it the identity transform.
    if (!_joint->has_transform()) {
      _joint->set_transform3d(LMatrix4d::ident_mat());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::get_num_frames
//       Access: Public, Virtual
//  Description: Returns the number of frames of animation for this
//               particular joint.
//
//               In the case of a EggJointNodePointer, which just
//               stores a pointer to a <Joint> entry for a character
//               model (not an animation table), there is always
//               exactly one frame: the rest pose.
////////////////////////////////////////////////////////////////////
int EggJointNodePointer::
get_num_frames() const {
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::get_frame
//       Access: Public, Virtual
//  Description: Returns the transform matrix corresponding to this
//               joint position in the nth frame.
//
//               In the case of a EggJointNodePointer, which just
//               stores a pointer to a <Joint> entry for a character
//               model (not an animation table), there is always
//               exactly one frame: the rest pose.
////////////////////////////////////////////////////////////////////
LMatrix4d EggJointNodePointer::
get_frame(int n) const {
  nassertr(n == 0, LMatrix4d::ident_mat());
  return _joint->get_transform3d();
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::set_frame
//       Access: Public, Virtual
//  Description: Sets the transform matrix corresponding to this
//               joint position in the nth frame.
//
//               In the case of a EggJointNodePointer, which just
//               stores a pointer to a <Joint> entry for a character
//               model (not an animation table), there is always
//               exactly one frame: the rest pose.
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
set_frame(int n, const LMatrix4d &mat) {
  nassertv(n == 0);
  _joint->set_transform3d(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::do_finish_reparent
//       Access: Protected
//  Description: Performs the actual reparenting operation
//               by removing the node from its old parent and
//               associating it with its new parent, if any.
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
do_finish_reparent(EggJointPointer *new_parent) {
  if (new_parent == (EggJointPointer *)NULL) {
    // No new parent; unparent the joint.
    EggGroupNode *egg_parent = _joint->get_parent();
    if (egg_parent != (EggGroupNode *)NULL) {
      egg_parent->remove_child(_joint.p());
      egg_parent->steal_children(*_joint);
    }

  } else {
    // Reparent the joint to its new parent (implicitly unparenting it
    // from its previous parent).
    EggJointNodePointer *new_node = DCAST(EggJointNodePointer, new_parent);
    if (new_node->_joint != _joint->get_parent()) {
      new_node->_joint->add_child(_joint.p());
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::move_vertices_to
//       Access: Public, Virtual
//  Description: Moves the vertices assigned to this joint into the
//               other joint (which should be of the same type).
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
move_vertices_to(EggJointPointer *new_joint) {
  if (new_joint == (EggJointPointer *)NULL) {
    _joint->unref_all_vertices();

  } else {
    EggJointNodePointer *new_node;
    DCAST_INTO_V(new_node, new_joint);

    new_node->_joint->steal_vrefs(_joint);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::do_rebuild
//       Access: Public, Virtual
//  Description: Rebuilds the entire table all at once, based on the
//               frames added by repeated calls to add_rebuild_frame()
//               since the last call to begin_rebuild().
//
//               Until do_rebuild() is called, the animation table is
//               not changed.
//
//               The return value is true if all frames are
//               acceptable, or false if there is some problem.
////////////////////////////////////////////////////////////////////
bool EggJointNodePointer::
do_rebuild(EggCharacterDb &db) {
  LMatrix4d mat;
  if (!db.get_matrix(this, EggCharacterDb::TT_rebuild_frame, 0, mat)) {
    // No rebuild frame; this is OK.
    return true;
  }

  _joint->set_transform3d(mat);

  // We shouldn't have a frame 1.
  nassertr(!db.get_matrix(this, EggCharacterDb::TT_rebuild_frame, 1, mat), false);
  
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::expose
//       Access: Public, Virtual
//  Description: Flags the joint with the indicated DCS flag so that
//               it will be loaded as a separate node in the player.
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
expose(EggGroup::DCSType dcs_type) {
  if (_joint != (EggGroup *)NULL) {
    _joint->set_dcs_type(dcs_type);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::apply_default_pose
//       Access: Public, Virtual
//  Description: Applies the pose from the indicated frame of the
//               indicated source joint as the initial pose for
//               this joint.
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
apply_default_pose(EggJointPointer *source_joint, int frame) {
  if (_joint != (EggGroup *)NULL) {
    LMatrix4d pose;
    if (frame >= 0 && frame < source_joint->get_num_frames()) {
      pose = source_joint->get_frame(frame);
    } else {
      pose = get_frame(0);
    }
    _joint->clear_default_pose();
    _joint->modify_default_pose().add_matrix4(pose);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::has_vertices
//       Access: Public, Virtual
//  Description: Returns true if there are any vertices referenced by
//               the node this points to, false otherwise.  For
//               certain kinds of back pointers (e.g. table animation
//               entries), this is always false.
////////////////////////////////////////////////////////////////////
bool EggJointNodePointer::
has_vertices() const {
  if (_joint != (EggGroup *)NULL) {
    return (_joint->vref_size() != 0) || _joint->joint_has_primitives();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::make_new_joint
//       Access: Public, Virtual
//  Description: Creates a new child of the current joint in the
//               egg data, and returns a pointer to it.
////////////////////////////////////////////////////////////////////
EggJointPointer *EggJointNodePointer::
make_new_joint(const string &name) {
  EggGroup *new_joint = new EggGroup(name);
  new_joint->set_group_type(EggGroup::GT_joint);
  _joint->add_child(new_joint);
  return new EggJointNodePointer(new_joint);
}

////////////////////////////////////////////////////////////////////
//     Function: EggJointNodePointer::set_name
//       Access: Public, Virtual
//  Description: Applies the indicated name change to the egg file.
////////////////////////////////////////////////////////////////////
void EggJointNodePointer::
set_name(const string &name) {
  _joint->set_name(name);
}
