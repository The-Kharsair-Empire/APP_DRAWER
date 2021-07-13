#define OLC_PGE_APPLICATION
//#include "olcPixelGameEngine.h"
#include "olcPGEX_UI.h"
#include "olcPixelGameEngine.h"
#include "olcPGEX_PopUp.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <list>
#include <fstream>


namespace kharsair
{
	struct Object;

	struct InteractionPoint
	{
		Object* parent;
		olc::vf2d pos;

	};

	struct Object
	{

		static float worldScale;
		static olc::vf2d worldOffset;

		void WorldToScreen(const olc::vf2d& v, int& screenX, int& screenY)
		{
			screenX = (int) ((v.x - worldOffset.x) * worldScale);
			screenY = (int) ((v.y - worldOffset.y) * worldScale);
		}

		virtual void draw_self(olc::PixelGameEngine* pge) = 0;

		virtual void DrawInteractionPoint(olc::PixelGameEngine* pge) = 0;

		virtual InteractionPoint* HitItrPoint(olc::vf2d& pos) = 0;

		virtual InteractionPoint* GetNextPoint(const olc::vf2d& pos) = 0;

		virtual bool clicked(const olc::vi2d& mousePos) = 0;

		virtual bool getObjectID() = 0; // use for tect editor

	};

	struct Node : public Object
	{

		std::vector<InteractionPoint> interaction_points;
		std::uint32_t max_interaction_point = 0;



		Node(olc::Pixel col) : color(col), radius(0), state_name("")
		{
		
			max_interaction_point = 2;
			interaction_points.reserve(max_interaction_point);
	
		}


		//info as app component
		std::string state_name;

		//structural info
		olc::vi2d center = {0, 0};
		std::int32_t radius;

		olc::Pixel color;

		bool getObjectID()
		{
			return 0;
		}



		bool clicked(const olc::vi2d& mousePos) override
		{

			return (mousePos - center).mag() < radius;
		}

		void draw_self(olc::PixelGameEngine* pge) override
		{
			int sx, sy, ex, ey;
			WorldToScreen(interaction_points[0].pos, sx, sy);
			WorldToScreen(interaction_points[1].pos, ex, ey);
			center = { sx, sy };
			radius = (interaction_points[0].pos - interaction_points[1].pos).mag() * worldScale;
	
			pge->FillCircle(center, radius, color);

			pge->DrawString({ center.x + 5, center.y + 5 }, state_name, olc::WHITE, 2U);
	

			DrawInteractionPoint(pge);
		}

		void DrawInteractionPoint(olc::PixelGameEngine* pge) override
		{
			for (auto& e : interaction_points)
			{
				int sx, sy;
				WorldToScreen(e.pos, sx, sy);
				pge->FillCircle(sx, sy, 2, olc::RED);
				pge->DrawCircle(sx, sy, 4, olc::VERY_DARK_GREY);
			}
		}

		InteractionPoint* HitItrPoint(olc::vf2d& pos) override
		{
			for (auto& p : interaction_points)
			{
				if ((pos - p.pos).mag() < 0.01f)
					return &p;
			}

			return nullptr;
		}

		void setPosition(olc::vf2d pos)
		{
			olc::vf2d diff = (interaction_points[1].pos - interaction_points[0].pos);
			interaction_points[0].pos = pos;
			interaction_points[1].pos = pos + diff;
		}

		InteractionPoint* GetNextPoint(const olc::vf2d& pos) override
		{
			if (interaction_points.size() == max_interaction_point) return nullptr;
			InteractionPoint itrp;
			itrp.parent = this;
			itrp.pos = pos;
			interaction_points.push_back(itrp);

			return &interaction_points[interaction_points.size() - 1];
		}

	};

	struct Edge : public Object
	{
	


		Edge(Node* s, Node* e, bool self_loop = false) : start(s), end(e), got_itrp(false), self_loop(self_loop), transition_name(""){}



		InteractionPoint itrp;
		bool self_loop;

		std::string transition_name;
		Node* start;
		Node* end;

		struct vStr2d
		{
			std::string key;
			std::string value;
		};

		struct Predicate
		{
			
			std::string predicate_name;
			std::vector<vStr2d> parameters;
		};

		std::vector<Predicate> guard;
		std::vector<Predicate> maintenance_goal;
		std::vector<Predicate> achievement_goal;


		bool got_itrp;

		olc::Pixel color = olc::WHITE;


		static float cos;
		static float sin;

		bool getObjectID()
		{
			return 1;
		}

		void draw_self(olc::PixelGameEngine* pge) override
		{

			int ex, ey;
		
			WorldToScreen(itrp.pos, ex, ey);
			olc::vi2d temp = { ex, ey };
			//pge->DrawLine(start->center, temp, color);
			pge->DrawLine(temp, end->center, color, 0xF0F0F0F0);

			if (!self_loop)
			{
				olc::vf2d op = start->center;
				olc::vf2d np = op;



				for (float t = 0; t < 1.0f; t += 0.01f)
				{
					np = (1 - t) * (1 - t) * (start->center) + 2 * (1 - t) * t * temp + t * t * (end->center);
					pge->DrawLine(op, np, color);
					op = np;
				}
			}
			else
			{
				olc::vf2d half_vec = (temp - start->center);
				olc::vf2d origin = end->center + half_vec;

				pge->DrawCircle(origin, half_vec.mag(), color);

			}

			pge->DrawString({ ex + 5, ey - 15 }, transition_name, olc::WHITE, 2U);
			pge->DrawString({ ex + 5, ey + 15 }, "Start State: " + start->state_name, olc::WHITE);
			pge->DrawString({ ex + 5, ey + 25 }, "END State: " + end->state_name, olc::WHITE);

	
			DrawInteractionPoint(pge);

		}

		bool clicked(const olc::vi2d& mousePos) override
		{
			return ((mousePos - itrp.pos).mag() < 0.01f);
		}

		InteractionPoint* HitItrPoint(olc::vf2d& pos) override
		{
			
			if ((pos - itrp.pos).mag() < 0.01f)
				return &itrp;
			
			return nullptr;
		}

		void DrawInteractionPoint(olc::PixelGameEngine* pge) override
		{

			int sx, sy;
			WorldToScreen(itrp.pos, sx, sy);
			pge->FillCircle(sx, sy, 2, olc::RED);

			pge->DrawCircle(sx, sy, 6, olc::YELLOW);
		
		}

		InteractionPoint* GetNextPoint(const olc::vf2d& pos) override
		{
			if (got_itrp) return nullptr;
			itrp.parent = this;
			itrp.pos = pos;
	
			got_itrp = true;
			return &itrp;
		}
	};

	float Object::worldScale = 1.0f;
	olc::vf2d Object::worldOffset = { 0, 0 };

	float Edge::cos = 0.866;
	float Edge::sin = 0.5;

}




class Drawer : public olc::PixelGameEngine
{
public:
	Drawer() 
	{

		sAppName = "APP Creator";
	}

private:
	olc::vf2d offset = { 0.0f, 0.0f };
	olc::vf2d startPan = { 0.0f, 0.0f };
	float scale = 10.0f;
	float grid = 1.0f;

	olc::UI_CONTAINER gui;

	kharsair::Node* n;
	kharsair::Edge* e;

	kharsair::Node* selected_node = nullptr;
	kharsair::Node* moving_node = nullptr;

	kharsair::InteractionPoint* selected_itrp = nullptr;

	olc::vf2d cursor = { 0, 0 };

	void WorldToScreen(const olc::vf2d& v, int& screenX, int& screenY)
	{
		screenX = (int) ((v.x - offset.x) * scale);
		screenY = (int) ((v.y - offset.y) * scale);
	}

	void ScreenToWorld(int screenX, int screenY, olc::vf2d& v)
	{
		v.x = (float) (screenX) / scale + offset.x;
		v.y = (float) (screenY) / scale + offset.y;
	}


	std::list<kharsair::Node*> list_of_states;
	std::list<kharsair::Edge*> list_of_transition;


	//kharsair::editorPanel editor;

	kharsair::Object* obj_being_edited = nullptr;


public:
	bool OnUserCreate() override 
	{
		obj_being_edited = nullptr;

		gui.addTextField(10, 70, 20);
		


		offset = {
			(float) (-ScreenWidth() / 2) / scale,
			(float) (-ScreenHeight() / 2) / scale
		};
	
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override 
	{

	

		gui.Update(fElapsedTime);
		
		olc::vf2d mousePos = { (float) GetMouseX(), (float) GetMouseY() };

		if (GetMouse(2).bPressed)
		{
			startPan = mousePos;
		}

		if (GetMouse(2).bHeld)
		{
			offset -= (mousePos - startPan) / scale;
			startPan = mousePos;
		}

		olc::vf2d mouseBfZoom;
		ScreenToWorld((int) mousePos.x, (int) mousePos.y, mouseBfZoom);

		//if (GetMouseWheel() > 0)
		//{
		//	scale *= 1.1f;
		//}

		//if (GetMouseWheel() < 0 && scale > 1.0f)
		//{
		//	scale *= 0.9f;
		//	
		//}

		olc::vf2d mouseAfZoom;
		ScreenToWorld((int) mousePos.x, (int) mousePos.y, mouseAfZoom);
		offset += (mouseBfZoom - mouseAfZoom);

		cursor.x = floorf((mouseAfZoom.x + 0.5f) * grid);
		cursor.y = floorf((mouseAfZoom.y + 0.5f) * grid);


		if (GetKey(olc::Key::N).bPressed && obj_being_edited == nullptr)
		{
			n = new kharsair::Node(olc::Pixel(rand()%255, rand() % 255, rand() % 255));

			selected_itrp = n->GetNextPoint(cursor);
			selected_itrp = n->GetNextPoint(cursor);
		}

		if (GetKey(olc::Key::I).bPressed && selected_itrp == nullptr && obj_being_edited == nullptr)
		{
			/*selected_itrp = nullptr;*/
			for (auto& node : list_of_states)
			{
				selected_itrp = node->HitItrPoint(cursor);

				if (selected_itrp != nullptr)
				{
					goto quit;
				}
			}

			for (auto& edge : list_of_transition)
			{
				selected_itrp = edge->HitItrPoint(cursor);
				if (selected_itrp != nullptr)
				{
					break;
				}
			}

		}

quit:

		if (selected_itrp != nullptr && obj_being_edited == nullptr)
		{
			selected_itrp->pos = cursor;
		}

		if (GetMouse(0).bReleased && obj_being_edited == nullptr)
		{
			if (n != nullptr)
			{
				selected_itrp = n->GetNextPoint(cursor);
				if (selected_itrp == nullptr)
				{
					list_of_states.push_back(n);
				}
				n = nullptr;
				
			}

			if (e != nullptr)
			{
				selected_itrp = nullptr;

			}
			
		}

		if (GetMouse(1).bPressed && selected_itrp == nullptr && obj_being_edited == nullptr)
		{
			for (auto& node : list_of_states)
			{
				if (node->clicked({GetMouseX(), GetMouseY()}))
				{
					if (selected_node == nullptr)
					{
						selected_node = node;
						break;
					}
					else
					{
						bool is_self_loop = false;
						if (selected_node == node) is_self_loop = true;
						e = new kharsair::Edge(selected_node, node, is_self_loop);
						selected_itrp = e->GetNextPoint(cursor);
						list_of_transition.push_back(e); 
						selected_node = nullptr;
						break;
					}
				}
			}
		}

		if (GetKey(olc::Key::SPACE).bPressed && selected_itrp == nullptr && obj_being_edited == nullptr)
		{
			for (auto& node : list_of_states)
			{
				if (node->clicked({ GetMouseX(), GetMouseY() }))
				{
					moving_node = node;
					break;
				}
			}

		}

		if (GetKey(olc::Key::SPACE).bHeld && moving_node != nullptr && selected_itrp == nullptr && obj_being_edited == nullptr)
		{
			moving_node->setPosition(cursor);
		}

		if (GetKey(olc::Key::SPACE).bReleased && selected_itrp == nullptr && obj_being_edited == nullptr)
		{
			moving_node = nullptr;
		}

		if (GetKey(olc::Key::E).bPressed)
		{
			if (obj_being_edited == nullptr)
			{
				for (auto& node : list_of_states)
				{
					if (node->clicked({ GetMouseX(), GetMouseY() }))
					{
						obj_being_edited = node;
						//n = nullptr;
						break;
					}
				}

				for (auto& edge : list_of_transition)
				{
					if (edge->clicked(cursor))
					{
						obj_being_edited = edge;
						//e = nullptr;
						break;
					}
				}
			}

		

		}

		


		if (GetKey(olc::Key::D).bPressed && selected_itrp == nullptr && moving_node == nullptr && selected_node == nullptr && obj_being_edited == nullptr)
		{
			for (auto& node : list_of_states)
			{
				if (node->clicked({ GetMouseX(), GetMouseY() }))
				{
					list_of_states.remove(node);
					//TODO: don't forget to delete node;
					n = nullptr;
					goto stop_check_1;
				}
			}

			for (auto& edge : list_of_transition)
			{
				if (edge->clicked(cursor))
				{

					list_of_transition.remove(edge);
					//TODO: don't forget to delete edge
					break;
				}
			}


		}

stop_check_1:

		Clear(olc::VERY_DARK_BLUE);
		
		int sx, sy;
		int ex, ey;

		olc::vf2d worldTopLeft, worldBottomRight;

		ScreenToWorld(0, 0, worldTopLeft);
		ScreenToWorld(ScreenWidth(), ScreenHeight(), worldBottomRight);

		worldTopLeft.x = floor(worldTopLeft.x);
		worldTopLeft.y = floor(worldTopLeft.y);
		worldBottomRight.x = ceil(worldBottomRight.x);
		worldBottomRight.y = ceil(worldBottomRight.y);


		for (float x = worldTopLeft.x; x < worldBottomRight.x; x += grid)
		{
			for (float y = worldTopLeft.y; y < worldBottomRight.y; y += grid)
			{
				WorldToScreen({ x, y }, sx, sy);
				Draw(sx, sy, olc::BLUE);
			}
		}

		WorldToScreen({ 0, worldTopLeft.y }, sx, sy);
		WorldToScreen({ 0, worldBottomRight.y }, ex, ey);
		DrawLine(sx, sy, ex, ey, olc::GREY, 0xFF00FF00);

		WorldToScreen({ worldTopLeft.x, 0 }, sx, sy);
		WorldToScreen({ worldBottomRight.x, 0 }, ex, ey);
		DrawLine(sx, sy, ex, ey, olc::GREY, 0xFF00FF00);

		kharsair::Object::worldOffset = offset;
		kharsair::Object::worldScale = scale;


		for (auto& e_e : list_of_transition)
		{
			e_e->draw_self(this);
			
		}

		for (auto& e_n : list_of_states)
		{
			e_n->draw_self(this);
		
		}


		if (n != nullptr)
		{
			n->draw_self(this);
		
		}
		

		WorldToScreen(cursor, sx, sy);
		DrawCircle(sx, sy, 3, olc::YELLOW);

		DrawString(10, 10, "X = " + std::to_string(cursor.x) + ", Y = " + std::to_string(cursor.y));

		if (obj_being_edited != nullptr)
		{
			if (obj_being_edited->getObjectID())
			{
				DrawString(10, 30, "Editing: " + ((kharsair::Edge*) obj_being_edited)->transition_name);
			}
			else
			{
				DrawString(10, 30, "Editing: " + ((kharsair::Node*) obj_being_edited)->state_name);
			}
			
			if (GetKey(olc::Key::ENTER).bPressed)
			{
				if (obj_being_edited->getObjectID()) // is edge
				{
					std::string s_n = gui.getTxtFieldStr(0);
					((kharsair::Edge*) obj_being_edited)->transition_name = s_n;
				}
				else
				{

					std::string s_n = gui.getTxtFieldStr(0);
					((kharsair::Node*) obj_being_edited)->state_name = s_n;

				}

				obj_being_edited = nullptr;

			}

		/*	if (GetKey(olc::Key::ENTER).bPressed)
			{
				obj_being_edited = nullptr;

			}*/
				

		}
		
		gui.drawUIObjects();

		if (GetKey(olc::Key::F).bReleased)
		{
			
			rapidjson::StringBuffer sb;
			rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

			
			writer.StartObject();
			{
				writer.Key("States");
				writer.StartArray();
				{
					for (auto& node : list_of_states)
						writer.String(node->state_name.c_str());
				}
				writer.EndArray();


				writer.Key("Transitions");
				writer.StartArray();
				{
					for (auto& edge : list_of_transition)
					{
						writer.StartObject();
						{
							writer.Key("name");         writer.String(edge->transition_name.c_str());
							writer.Key("StartState");   writer.String(edge->start->state_name.c_str());
							writer.Key("EndState");     writer.String(edge->end->state_name.c_str());

							writer.Key("Guards");
							writer.StartArray();
							{
								for (auto& g : edge->guard)
								{
									writer.StartObject();
									writer.EndObject();

								}
							}
							writer.EndArray();

							writer.Key("MaintenanceGoals");
							writer.StartArray();
							{
								for (auto& m : edge->maintenance_goal)
								{
									writer.StartObject();
									writer.EndObject();
								}
							}
							writer.EndArray();

							writer.Key("AchievementGoals");
							writer.StartArray();
							{
								for (auto& a : edge->achievement_goal)
								{
									writer.StartObject();
									writer.EndObject();
								}
							}
							writer.EndArray();
						}
						writer.EndObject();
					}
				}
				writer.EndArray();
			}
			writer.EndObject();

			std::cout << sb.GetString() << std::endl;
			
			
			
			

		}

		
		return true;
	}
};

int main() 
{
	Drawer demo;
	if (demo.Construct(1920, 1080, 1, 1))
		demo.Start();
	return 0;
}

