/*************************************************************************/
/*  tile_map_editor_plugin.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "tile_map_editor_plugin.h"
#include "tools/editor/editor_node.h"
#include "os/keyboard.h"

#include "canvas_item_editor_plugin.h"
#include "os/file_access.h"
#include "tools/editor/editor_settings.h"
#include "os/input.h"


void TileMapEditor::_notification(int p_what) {

	switch(p_what) {

		case NOTIFICATION_READY: {

			pane_drag->connect("dragged", this,"_pane_drag");
			mirror_x->set_icon( get_icon("MirrorX","EditorIcons"));
			mirror_y->set_icon( get_icon("MirrorY","EditorIcons"));

		} break;
	}

}

void TileMapEditor::_canvas_mouse_enter()  {

	mouse_over=true;
	canvas_item_editor->update();


}

void TileMapEditor::_canvas_mouse_exit()  {

	mouse_over=false;
	canvas_item_editor->update();

}

int TileMapEditor::get_selected_tile() const {

	TreeItem *item = palette->get_selected();
	if (!item)
		return TileMap::INVALID_CELL;
	return item->get_metadata(0);
}

void TileMapEditor::_set_cell(const Point2i& p_pos,int p_value,bool p_flip_h, bool p_flip_v) {

	ERR_FAIL_COND(!node);

	bool prev_flip_h=node->is_cell_x_flipped(p_pos.x,p_pos.y);
	bool prev_flip_v=node->is_cell_y_flipped(p_pos.x,p_pos.y);
	int prev_val=node->get_cell(p_pos.x,p_pos.y);

	if (p_value==prev_val && p_flip_h==prev_flip_h && p_flip_v==prev_flip_v)
		return; //check that it's actually different

	node->set_cell(p_pos.x,p_pos.y,p_value,p_flip_h,p_flip_v);

#if 0
//not yet
	undo_redo->create_action("Set Tile");
	undo_redo->add_do_method(node,"set_cell",p_pos.x,p_pos.y,p_value,p_flip_h,p_flip_v);
	undo_redo->add_undo_method(node,"set_cell",p_pos.x,p_pos.y,prev_val,prev_flip_h,prev_flip_v);
	undo_redo->commit_action();
#endif

}

void TileMapEditor::_update_palette() {

	if (!node)
		return;

	palette->clear();;

	Ref<TileSet> tileset=node->get_tileset();
	if (!tileset.is_valid())
		return;


	TreeItem *root = palette->create_item();
	palette->set_hide_root(true);
	List<int> tiles;
	tileset->get_tile_list(&tiles);

	for(List<int>::Element *E=tiles.front();E;E=E->next()) {

		TreeItem *tile = palette->create_item(root);

		tile->set_icon_max_width(0,64);
		Ref<Texture> tex = tileset->tile_get_texture(E->get());
		if (tex.is_valid()) {
			tile->set_icon(0,tex);
			Rect2 region = tileset->tile_get_region(E->get());
			if (region!=Rect2())
				tile->set_icon_region(0,region);

		} else if (tileset->tile_get_name(E->get())!="")
			tile->set_text(0,tileset->tile_get_name(E->get()));
		else
			tile->set_text(0,"#"+itos(E->get()));

		tile->set_metadata(0,E->get());

	}
}

void TileMapEditor::_node_removed(Node *p_node) {

	if(p_node==node) {
		node=NULL;
		hide();
	}

}
void TileMapEditor::_menu_option(int p_option) {

	switch(p_option) {


	}
}

struct _TileMapEditorCopyData {
	Point2i pos;
	int cell;
	bool flip_h;
	bool flip_v;
};

bool TileMapEditor::forward_input_event(const InputEvent& p_event) {

	if (!node || !node->get_tileset().is_valid())
		return false;

	Matrix32 xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * node->get_global_transform();
	Matrix32 xform_inv = xform.affine_inverse();
	Vector2 snap = Vector2(1,1)*node->get_cell_size();


	switch(p_event.type) {

		case InputEvent::MOUSE_BUTTON: {

			const InputEventMouseButton &mb=p_event.mouse_button;

			if (mb.button_index==BUTTON_LEFT) {


				if (mb.pressed && tool==TOOL_DUPLICATING) {


					List<_TileMapEditorCopyData> dupdata;
					Point2 ofs = over_tile-selection.pos;

					for(int i=selection.pos.y;i<=selection.pos.y+selection.size.y;i++) {

						for(int j=selection.pos.x;j<=selection.pos.x+selection.size.x;j++) {

							_TileMapEditorCopyData tcd;
							tcd.pos=Point2i(j,i);
							tcd.cell=node->get_cell(j,i);
							tcd.flip_h=node->is_cell_x_flipped(j,i);
							tcd.flip_v=node->is_cell_y_flipped(j,i);
							dupdata.push_back(tcd);


						}
					}

					for (List<_TileMapEditorCopyData>::Element *E=dupdata.front();E;E=E->next()) {


						_set_cell(E->get().pos+ofs,E->get().cell,E->get().flip_h,E->get().flip_v);
					}

					tool=TOOL_NONE;
					canvas_item_editor->update();
					selection.pos=over_tile;

				} else if (mb.pressed && tool==TOOL_NONE) {

					if (Input::get_singleton()->is_key_pressed(KEY_SPACE))
						return false; //drag
					if (mb.mod.shift) {

						tool=TOOL_SELECTING;
						selection_begin =(xform_inv.xform(Point2(mb.x,mb.y))/snap).floor();
						selection.pos=selection_begin;
						selection.size=Point2(0,0);
						selection_active=true;
						canvas_item_editor->update();
						return true;

					} else {
						int id = get_selected_tile();
						if (id!=TileMap::INVALID_CELL) {
							tool=TOOL_PAINTING;
							Point2i local =(xform_inv.xform(Point2(mb.x,mb.y))/snap).floor();
							paint_undo.clear();
							CellOp op;
							op.idx = node->get_cell(local.x,local.y);
							if (op.idx>=0) {
								if (node->is_cell_x_flipped(local.x,local.y))
									op.xf=true;
								if (node->is_cell_y_flipped(local.x,local.y))
									op.yf=true;
							}
							paint_undo[local]=op;
							node->set_cell(local.x,local.y,id,mirror_x->is_pressed(),mirror_y->is_pressed());
							return true;
						}
					}
				} else {

					if (tool==TOOL_PAINTING || tool == TOOL_SELECTING) {

						if (tool==TOOL_PAINTING) {

							if (paint_undo.size()) {
								undo_redo->create_action("Paint TileMap");
								for(Map<Point2i,CellOp>::Element *E=paint_undo.front();E;E=E->next()) {

									Point2i p=E->key();
									undo_redo->add_do_method(node,"set_cell",p.x,p.y,node->get_cell(p.x,p.y),node->is_cell_x_flipped(p.x,p.y),node->is_cell_y_flipped(p.x,p.y));
									undo_redo->add_undo_method(node,"set_cell",p.x,p.y,E->get().idx,E->get().xf,E->get().yf);
								}

								undo_redo->commit_action();
							}
						}
						tool=TOOL_NONE;
						return true;
					}
				}
			}

			if (mb.button_index==BUTTON_RIGHT) {

				if (mb.pressed && tool==TOOL_DUPLICATING) {

					tool=TOOL_NONE;
					canvas_item_editor->update();
				} else if (mb.pressed && tool==TOOL_NONE) {

					tool=TOOL_ERASING;
					Point2i local =(xform_inv.xform(Point2(mb.x,mb.y))/snap).floor();
					_set_cell(local,TileMap::INVALID_CELL);
					return true;
				} else {

					if (tool==TOOL_ERASING) {

						tool=TOOL_NONE;
						return true;
					}
				}
			}

		} break;
		case InputEvent::MOUSE_MOTION: {

			const InputEventMouseMotion &mm=p_event.mouse_motion;

			Point2i new_over_tile = (xform_inv.xform(Point2(mm.x,mm.y))/snap).floor();
			if (new_over_tile!=over_tile) {

				over_tile=new_over_tile;
				canvas_item_editor->update();
			}



			if (tool==TOOL_PAINTING) {

				int id = get_selected_tile();
				if (id!=TileMap::INVALID_CELL) {

					if (!paint_undo.has(over_tile)) {

						CellOp op;
						op.idx = node->get_cell(over_tile.x,over_tile.y);
						if (op.idx>=0) {
							if (node->is_cell_x_flipped(over_tile.x,over_tile.y))
								op.xf=true;
							if (node->is_cell_y_flipped(over_tile.x,over_tile.y))
								op.yf=true;
						}
						paint_undo[over_tile]=op;
					}
					node->set_cell(over_tile.x,over_tile.y,id,mirror_x->is_pressed(),mirror_y->is_pressed());

					return true;
				}
			}

			if (tool==TOOL_SELECTING) {

				Point2i begin=selection_begin;
				Point2i end =over_tile;

				if (begin.x > end.x) {

					SWAP( begin.x, end.x);
				}
				if (begin.y > end.y) {

					SWAP( begin.y, end.y);
				}

				selection.pos=begin;
				selection.size=end-begin;
				canvas_item_editor->update();

				return true;

			}
			if (tool==TOOL_ERASING) {
				Point2i local =over_tile;
				_set_cell(local,TileMap::INVALID_CELL);
				return true;
			}


		} break;
		case InputEvent::KEY: {

			const InputEventKey &k = p_event.key;
			if (!node)
				break;

			if (k.pressed && k.scancode==KEY_DELETE && selection_active && tool==TOOL_NONE) {

				for(int i=selection.pos.y;i<=selection.pos.y+selection.size.y;i++) {

					for(int j=selection.pos.x;j<=selection.pos.x+selection.size.x;j++) {


						_set_cell(Point2i(j,i),TileMap::INVALID_CELL);
					}
				}

				selection_active=false;
				canvas_item_editor->update();
				return true;
			}

			if (mouse_over && k.pressed && k.scancode==KEY_A  && tool==TOOL_NONE) {

				/*int cell = node->get_cell(over_tile.x,over_tile.y);
				if (cell!=TileMap::INVALID_CELL) {
					bool flip_h = node->is_cell_x_flipped(over_tile.x,over_tile.y);
					bool flip_v = node->is_cell_y_flipped(over_tile.x,over_tile.y);
					_set_cell(over_tile,cell,!flip_h,flip_v);
				}*/

				mirror_x->set_pressed( ! mirror_x->is_pressed() );
				canvas_item_editor->update();
				return true;
			}
			if (mouse_over && k.pressed && k.scancode==KEY_S  && tool==TOOL_NONE) {


				/*
				int cell = node->get_cell(over_tile.x,over_tile.y);
				if (cell!=TileMap::INVALID_CELL) {

					bool flip_h = node->is_cell_x_flipped(over_tile.x,over_tile.y);
					bool flip_v = node->is_cell_y_flipped(over_tile.x,over_tile.y);
					_set_cell(over_tile,cell,flip_h,!flip_v);
				}*/

				mirror_y->set_pressed( ! mirror_y->is_pressed() );
				canvas_item_editor->update();
				return true;
			}

			if (mouse_over && selection_active && k.pressed && k.mod.command && k.scancode==KEY_D && tool==TOOL_NONE) {

				tool=TOOL_DUPLICATING;
				canvas_item_editor->update();
				return true;
			}



		} break;
	}

	return false;
}
void TileMapEditor::_canvas_draw() {

	if (!node)
		return;

	int cell_size=node->get_cell_size();

	Matrix32 xform = CanvasItemEditor::get_singleton()->get_canvas_transform() * node->get_global_transform();
	Matrix32 xform_inv = xform.affine_inverse();


	Size2 screen_size=canvas_item_editor->get_size();
	Rect2 aabb;
	aabb.pos=xform_inv.xform(Vector2());
	aabb.expand_to(xform_inv.xform(Vector2(0,screen_size.height)));
	aabb.expand_to(xform_inv.xform(Vector2(screen_size.width,0)));
	aabb.expand_to(xform_inv.xform(screen_size));
	Rect2i si=aabb;

	for(int i=(si.pos.x/cell_size)-1;i<=(si.pos.x+si.size.x)/cell_size;i++) {

		int ofs = i*cell_size;

		Color col=i==0?Color(1,0.8,0.2,0.5):Color(1,0.3,0.1,0.2);
		canvas_item_editor->draw_line(xform.xform(Point2(ofs,si.pos.y)),xform.xform(Point2(ofs,si.pos.y+si.size.y)),col,1);

	}

	for(int i=(si.pos.y/cell_size)-1;i<=(si.pos.y+si.size.y)/cell_size;i++) {

		int ofs = i*cell_size;
		Color col=i==0?Color(1,0.8,0.2,0.5):Color(1,0.3,0.1,0.2);
		canvas_item_editor->draw_line(xform.xform(Point2(si.pos.x,ofs)),xform.xform(Point2(si.pos.x+si.size.x,ofs)),col,1);
	}


	if (selection_active) {

		Vector<Vector2> points;
		points.push_back( xform.xform( selection.pos * cell_size) );
		points.push_back( xform.xform( (selection.pos+Point2(selection.size.x+1,0)) * cell_size) );
		points.push_back( xform.xform( (selection.pos+Point2(selection.size.x+1,selection.size.y+1)) * cell_size) );
		points.push_back( xform.xform( (selection.pos+Point2(0,selection.size.y+1)) * cell_size) );
		Color col=Color(0.2,0.8,1,0.4);

		canvas_item_editor->draw_colored_polygon(points,col);
	}


	if (mouse_over){

		const Vector2 endpoints[4]={

			xform.xform( over_tile * cell_size) ,
			xform.xform( (over_tile+Point2(1,0)) * cell_size) ,
			xform.xform( (over_tile+Point2(1,1)) * cell_size) ,
			xform.xform( (over_tile+Point2(0,1)) * cell_size) ,


		};
		Color col;
		if (node->get_cell(over_tile.x,over_tile.y)!=TileMap::INVALID_CELL)
			col=Color(0.2,0.8,1.0,0.8);
		else
			col=Color(1.0,0.4,0.2,0.8);

		for(int i=0;i<4;i++)
			canvas_item_editor->draw_line(endpoints[i],endpoints[(i+1)%4],col,2);



		if (tool==TOOL_DUPLICATING) {

			Rect2i duplicate=selection;
			duplicate.pos=over_tile;


			Vector<Vector2> points;
			points.push_back( xform.xform( duplicate.pos * cell_size) );
			points.push_back( xform.xform( (duplicate.pos+Point2(duplicate.size.x+1,0)) * cell_size) );
			points.push_back( xform.xform( (duplicate.pos+Point2(duplicate.size.x+1,duplicate.size.y+1)) * cell_size) );
			points.push_back( xform.xform( (duplicate.pos+Point2(0,duplicate.size.y+1)) * cell_size) );
			Color col=Color(0.2,1.0,0.8,0.4);

			canvas_item_editor->draw_colored_polygon(points,col);

		} else {

			Ref<TileSet> ts = node->get_tileset();


			if (ts.is_valid()) {

				int st = get_selected_tile();
				if (ts->has_tile(st)) {

					Ref<Texture> t = ts->tile_get_texture(st);
					if (t.is_valid()) {
						Rect2 r = ts->tile_get_region(st);
						Size2 sc = (endpoints[2]-endpoints[0])/cell_size;
						if (mirror_x->is_pressed())
							sc.x*=-1.0;
						if (mirror_y->is_pressed())
							sc.y*=-1.0;
						if (r==Rect2()) {

							canvas_item_editor->draw_texture_rect(t,Rect2(endpoints[0],t->get_size()*sc),false,Color(1,1,1,0.5));
						} else {

							canvas_item_editor->draw_texture_rect_region(t,Rect2(endpoints[0],r.get_size()*sc),r,Color(1,1,1,0.5));
						}
					}
				}
			}

		}
	}



}



void TileMapEditor::edit(Node *p_tile_map) {

	if (!canvas_item_editor) {
		canvas_item_editor=CanvasItemEditor::get_singleton()->get_viewport_control();
	}

	if (p_tile_map) {

		node=p_tile_map->cast_to<TileMap>();
		if (!canvas_item_editor->is_connected("draw",this,"_canvas_draw"))
			canvas_item_editor->connect("draw",this,"_canvas_draw");
		if (!canvas_item_editor->is_connected("mouse_enter",this,"_canvas_mouse_enter"))
			canvas_item_editor->connect("mouse_enter",this,"_canvas_mouse_enter");
		if (!canvas_item_editor->is_connected("mouse_exit",this,"_canvas_mouse_exit"))
			canvas_item_editor->connect("mouse_exit",this,"_canvas_mouse_exit");

		_update_palette();

	} else {
		node=NULL;

		if (canvas_item_editor->is_connected("draw",this,"_canvas_draw"))
			canvas_item_editor->disconnect("draw",this,"_canvas_draw");
		if (canvas_item_editor->is_connected("mouse_enter",this,"_canvas_mouse_enter"))
			canvas_item_editor->disconnect("mouse_enter",this,"_canvas_mouse_enter");
		if (canvas_item_editor->is_connected("mouse_exit",this,"_canvas_mouse_exit"))
			canvas_item_editor->disconnect("mouse_exit",this,"_canvas_mouse_exit");

		_update_palette();
	}


}

void TileMapEditor::_pane_drag(const Point2& p_to) {

	int x = theme_panel->get_margin(MARGIN_RIGHT);

	x+=p_to.x;
	if (x<10)
		x=10;
	if (x>300)
		x=300;
	theme_panel->set_margin(MARGIN_RIGHT,x);
}

void TileMapEditor::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("_menu_option"),&TileMapEditor::_menu_option);
	ObjectTypeDB::bind_method(_MD("_canvas_draw"),&TileMapEditor::_canvas_draw);
	ObjectTypeDB::bind_method(_MD("_pane_drag"),&TileMapEditor::_pane_drag);
	ObjectTypeDB::bind_method(_MD("_canvas_mouse_enter"),&TileMapEditor::_canvas_mouse_enter);
	ObjectTypeDB::bind_method(_MD("_canvas_mouse_exit"),&TileMapEditor::_canvas_mouse_exit);

}

TileMapEditor::TileMapEditor(EditorNode *p_editor) {

	canvas_item_editor=NULL;
	editor=p_editor;
	undo_redo = editor->get_undo_redo();

	theme_panel = memnew( Panel );
	theme_panel->set_anchor(MARGIN_BOTTOM,ANCHOR_END);
	theme_panel->set_begin( Point2(0,26));
	theme_panel->set_end( Point2(100,0) );
	p_editor->get_viewport()->add_child(theme_panel);
	theme_panel->hide();

	palette = memnew( Tree );
	palette->set_area_as_parent_rect(4);
	palette->set_margin(MARGIN_TOP,25);;
	theme_panel->add_child(palette);

	pane_drag = memnew( PaneDrag )	;
	pane_drag->set_anchor(MARGIN_LEFT,ANCHOR_END);
	pane_drag->set_begin(Point2(16,4));
	theme_panel->add_child(pane_drag);

	add_child( memnew( VSeparator ));

	mirror_x = memnew( ToolButton );
	mirror_x->set_toggle_mode(true);
	mirror_x->set_tooltip(_TR("Mirror X (A)"));
	mirror_x->set_focus_mode(FOCUS_NONE);
	add_child(mirror_x);
	mirror_y = memnew( ToolButton );
	mirror_y->set_toggle_mode(true);
	mirror_y->set_tooltip(_TR("Mirror Y (S)"));
	mirror_y->set_focus_mode(FOCUS_NONE);
	add_child(mirror_y);


	tool=TOOL_NONE;
	selection_active=false;
	mouse_over=false;
}


void TileMapEditorPlugin::edit(Object *p_object) {

	tile_map_editor->edit(p_object->cast_to<Node>());
}

bool TileMapEditorPlugin::handles(Object *p_object) const {

	return p_object->is_type("TileMap");
}

void TileMapEditorPlugin::make_visible(bool p_visible) {

	if (p_visible) {
		tile_map_editor->show();
		tile_map_editor->theme_panel->show();

	} else {

		tile_map_editor->hide();
		tile_map_editor->theme_panel->hide();
		tile_map_editor->edit(NULL);
	}

}

TileMapEditorPlugin::TileMapEditorPlugin(EditorNode *p_node) {

	editor=p_node;
	tile_map_editor = memnew( TileMapEditor(p_node) );
	CanvasItemEditor::get_singleton()->add_control_to_menu_panel(tile_map_editor);

	tile_map_editor->hide();




}


TileMapEditorPlugin::~TileMapEditorPlugin()
{
}

