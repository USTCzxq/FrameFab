#include"ProcessAnalyzer.h"


ProcessAnalyzer::ProcessAnalyzer(FiberPrintPlugIn	*ptr_fiberprint)
{

	ptr_fiberprint_ = ptr_fiberprint;
    break_height_ = 2; //2cm
	Init();


	SetThick();

	Write();
	
}
ProcessAnalyzer::ProcessAnalyzer(WireFrame * ptr_frame_)
{

	int ttd[85] = { 151, 147, 9, 1, 41, 159, 59, 167, 99, 153, 61, 57, 71, 69, 43, 45, 3, 5, 7, 139, 33, 31, 55, 95,
		97, 155, 73, 133, 11, 21, 29, 53,
		137, 37, 39, 141, 19, 17, 15, 13, 135, 81, 83, 63, 65, 157, 105, 103,
		35, 85, 75, 107, 161, 67, 127, 113,
		101, 89, 87, 79, 77, 111, 109, 119, 91
		, 121, 125, 123, 169, 131, 129, 117, 115, 143, 149, 165, 163, 93, 49, 91, 51, 25, 27, 23, 47
	};

	FILE *s = fopen("I:\\Users\\Alienware\\Desktop\\Result\\Start.txt", "w");

	FILE *e = fopen("I:\\Users\\Alienware\\Desktop\\Result\\End.txt", "w");

	vector<point> S;
	vector<point> E;

	for (int i = 0; i < 85; i++)
	{
		WF_edge* temp = ptr_frame_->GetEdge(ttd[i]);
		point s_ = temp->pvert_->Position();
		point e_ = temp->ppair_->pvert_->Position();


		point s_p; point e_p;
		if (i == 0)
		{
			if (s_ == ptr_frame_->GetEdge(ttd[i+1])->pvert_->Position() || s_ == ptr_frame_->GetEdge(ttd[i+1])->ppair_->pvert_->Position())
			{
				S.push_back(e_);
				E.push_back(s_);
				
			}
			else
			{
				S.push_back(s_);
				E.push_back(e_);
			
			}
			continue;
		}

		if (s_ == E[E.size() - 1])
		{
			S.push_back(s_);
			E.push_back(e_);
			continue;
		}

		if (e_ == E[E.size() - 1])
		{
			S.push_back(e_);
			E.push_back(s_);
			continue;
		}

		if (s_ == ptr_frame_->GetEdge(ttd[i + 1])->pvert_->Position() || s_ == ptr_frame_->GetEdge(ttd[i + 1])->ppair_->pvert_->Position())
		{
			S.push_back(e_);
			E.push_back(s_);

		}
		else
		{
			S.push_back(s_);
			E.push_back(e_);

		}
		

	}




	for (int i = 0; i < 85; i++)
	{
		
		point s_ = S[i];

		point e_ = E[i];

		fprintf(s, "%lf ,%lf ,%lf", s_.x(), s_.y(), s_.z());
		fprintf(s, "\n");
		fprintf(e, "%lf ,%lf ,%lf", e_.x(), e_.y(), e_.z());
		fprintf(e, "\n");

	}

	std::fclose(s);
	std::fclose(e);


}


vector<Process*> * ProcessAnalyzer::Init()
{

	
	print_ = new vector<Process*>();
	exist_point_.clear();



	if (ptr_fiberprint_ == NULL)
	{
		cout << "Error: sequanalyer=NULL." << endl;
		return NULL;
	}

	const std::vector<QueueInfo> print_queue = *(ptr_fiberprint_->GetQueue());

	if (print_queue.size() == 0)
	{
		cout << "Error: query number==0." << endl;
	}
	
	for (int i = 0; i <print_queue.size(); i++)
	{

		QueueInfo temp_info = print_queue[i];
        int dual_id = print_queue[i].dual_id_;
     	WF_edge *e = ptr_fiberprint_->ptr_frame_->GetEdge((*(ptr_fiberprint_->GetDualVertList()))[dual_id]->orig_id());
		
		if (i == 0)
		{
			Process* temp_process = new Process();
			if (e->pvert_->Position().z() >= e->ppair_->pvert_->Position().z())
			{
				temp_process->start_ = e->ppair_->pvert_->Position();
				temp_process->end_ = e->pvert_->Position();
			}
			else
			{
				temp_process->end_ = e->ppair_->pvert_->Position();
				temp_process->start_ = e->pvert_->Position();
			}
			temp_process->extruder_state_ = true;
			temp_process->fan_state_ = true;
			temp_process->move_state_ = 5;			
			temp_process->vector = point(0, 0, 1);
			print_->push_back(temp_process);
			exist_point_.push_back(temp_process->start_);
			exist_point_.push_back(temp_process->end_);
			continue;
		}

		Process* temp_process = new Process();
		temp_process = SetPoint(e, i);
		temp_process = SetFan(temp_process);
		temp_process = SetExtruderSpeed(temp_process,i);
	
		if (temp_process->start_ != (*print_)[print_->size() - 1]->end_)
			SetBreak(temp_process);

		if (!IfPointInVector(temp_process->start_))
			exist_point_.push_back(temp_process->start_);
		if (!IfPointInVector(temp_process->end_))
			exist_point_.push_back(temp_process->end_);

		print_->push_back(temp_process);

	}

	return print_;

}


Process* ProcessAnalyzer::SetPoint(WF_edge *e, int id)
{
	
	Process* temp_process = new Process();
	point up, down;
	//Must
	Geometry::Vector3d vec;
	if (e->pvert_->Position().z() > e->ppair_->pvert_->Position().z())
	{
        vec=Geometry::Vector3d (e->pvert_->Position() - e->ppair_->pvert_->Position());
		up = e->ppair_->pvert_->Position();
		down = e->pvert_->Position();
	}
	else
	{
		 vec=Geometry::Vector3d(-e->pvert_->Position() + e->ppair_->pvert_->Position());
		 down = e->ppair_->pvert_->Position();
		 up = e->pvert_->Position();
	}
	



	if (Geometry::angle(vec, Geometry::Vector3d(0, 0, 1))<extruder_.Angle())
	{
		if (e->pvert_->Position().z() < e->ppair_->pvert_->Position().z())
		{
			temp_process->start_ = e->pvert_->Position();
			temp_process->end_ = e->ppair_->pvert_->Position();
		}
		else
		{
			temp_process->end_ = e->pvert_->Position();
			temp_process->start_ = e->ppair_->pvert_->Position();
		}
		return temp_process;
	}


	if (!IfPointInVector(e->ppair_->pvert_->Position()) && !IfPointInVector(e->pvert_->Position()))
	{
		temp_process->end_ =up;
		temp_process->start_ = down;
		return temp_process;
	}

	
	


	//if (e->pvert_->Position().z() == 0)
	//{
	//	temp_process->start_ = e->pvert_->Position();
	//	temp_process->end_ = e->ppair_->pvert_->Position();
	//	return temp_process;
	//}
	//
	//if (e->ppair_->pvert_->Position().z() == 0)
	//{
	//	temp_process->end_ = e->pvert_->Position();
	//	temp_process->start_ = e->ppair_->pvert_->Position();
	//	return temp_process;
	//}

	if (IfPointInVector(e->ppair_->pvert_->Position()) && !IfPointInVector(e->pvert_->Position()))
	{
		temp_process->end_ = e->pvert_->Position();
		temp_process->start_ = e->ppair_->pvert_->Position();
		return temp_process;
	}

	if (!IfPointInVector(e->ppair_->pvert_->Position()) && IfPointInVector(e->pvert_->Position()))
	{
		temp_process->start_ = e->pvert_->Position();
		temp_process->end_ = e->ppair_->pvert_->Position();
		return temp_process;
	}



	// Optimization

	Process * prior_process = (*print_)[print_->size() - 1];
	if (prior_process->end_ == e->pvert_->Position())
	{
		temp_process->start_ = e->pvert_->Position();
		temp_process->end_ = e->ppair_->pvert_->Position();
		return temp_process;
	}
	
	if (prior_process->end_ == e->ppair_->pvert_->Position())
	{
		temp_process->end_ = e->pvert_->Position();
		temp_process->start_ = e->ppair_->pvert_->Position();
		return temp_process;
	}

	if (id == (*(ptr_fiberprint_->GetQueue())).size() - 1)
	{
		temp_process->end_ = up;
		temp_process->start_ = down;
		return temp_process;
	}
	
	
	

	QueueInfo temp_info = (*(ptr_fiberprint_->GetQueue()))[id+1];
	int dual_id = temp_info.dual_id_;
	WF_edge *later_e = ptr_fiberprint_->ptr_frame_->GetEdge((*(ptr_fiberprint_->GetDualVertList()))[dual_id]->orig_id());

	if (later_e->pvert_->Position() == e->pvert_->Position())
	{
		temp_process->end_ = e->pvert_->Position();
		temp_process->start_ = e->ppair_->pvert_->Position();
		return temp_process;
	}

	if (later_e->ppair_->pvert_->Position() == e->pvert_->Position())
	{
		temp_process->end_ = e->pvert_->Position();
		temp_process->start_ = e->ppair_->pvert_->Position();
		return temp_process;
	}

	if (later_e->pvert_->Position() == e->ppair_-> pvert_->Position())
	{
		temp_process->end_ = e->ppair_->pvert_->Position();
		temp_process->start_ = e->pvert_->Position();
		return temp_process;
	}

	if (later_e->ppair_-> pvert_->Position() == e->ppair_->pvert_->Position())
	{
		temp_process->end_ = e->ppair_->pvert_->Position();
		temp_process->start_ = e->pvert_->Position();
		return temp_process;
	}


	temp_process->start_ = down;
	temp_process->end_ = up;
	return temp_process;

}

bool  ProcessAnalyzer::IfPointInVector(point p)
{
	for (int i = 0; i <exist_point_.size(); i++)
	{
		if (exist_point_[i] == p)
			return true;
	}
	return false;
}

Process*   ProcessAnalyzer::SetFan(Process *temp)
{
	Process* temp_process = temp;

	if (IfPointInVector(temp_process->end_))
		temp_process->fan_state_ = false;
	else
		temp_process->fan_state_ = true;

	return temp_process;
}

Process*  ProcessAnalyzer::SetExtruderSpeed(Process* temp, int id)
{

	Process* temp_process = temp;

	temp_process->extruder_state_ = true;


	Geometry::Vector3d vec = Geometry::Vector3d(temp_process->end_ - temp_process->start_);
	double angle = Geometry::angle(vec, Geometry::Vector3d(0, 0, 1));

	if (abs(angle) < eps)
	{
		
	if((temp->end_ == point(0, 0, 0)) || temp->start_ == point(0, 0, 0))
		{
			temp_process->move_state_ = 6;
		}
	else
		temp_process->move_state_ = 5;

	}


		
	else if (abs(angle - pi / 2) < 0.1)
		temp_process->move_state_ = 4;
	else if (0 < angle&&angle < pi / 2)
	{
		if (temp_process->fan_state_ == 1)
			temp_process->move_state_ = 2;
		else
			temp_process->move_state_ = 7;

	}
		
	else if (angle>pi/2)
		temp_process->move_state_ = 3;

	temp_process->vector = ptr_fiberprint_->ptr_seqanalyzer_->GetExtru(id).Normal();
	return temp_process;
}



void ProcessAnalyzer::SetBreak(Process* temp)
{
	
	//0
	Process* up_process = new Process();
	up_process->start_ =   (*print_)[print_->size() - 1]->end_;
	up_process->end_ = up_process->start_ + point(0,  break_height_,0);

	up_process->fan_state_ = true;
	up_process->extruder_state_ = false;
	up_process->move_state_ = 0;
	up_process->vector = (*print_)[print_->size() - 1]->vector;

	print_->push_back(up_process);

	//1
	Process* mid_process_0 = new Process();
	mid_process_0->start_ = (*print_)[print_->size() - 1]->end_;
	mid_process_0->end_ = mid_process_0->start_ + point(0, 0,break_height_);

	mid_process_0->fan_state_ = false;
	mid_process_0->extruder_state_ = false;
	mid_process_0->move_state_ = -1;
	mid_process_0->vector = (*print_)[print_->size() - 1]->vector;

	print_->push_back(mid_process_0);

	//2
	Process* mid_process_1 = new Process();
	mid_process_1->start_ = (*print_)[print_->size() - 1]->end_;
	mid_process_1->end_ = temp->start_;
	mid_process_1->end_.z() = mid_process_1->start_.z();

	mid_process_1->fan_state_ = false;
	mid_process_1->extruder_state_ = false;
	mid_process_1->move_state_ = -1;
	mid_process_1->vector = (*print_)[print_->size() - 1]->vector;

	print_->push_back(mid_process_1);

	//3
	Process* down_process = new Process();

	down_process->start_ = (*print_)[print_->size() - 1]->end_;
	down_process->end_ =temp->start_;

	down_process->fan_state_ = false;
	down_process->extruder_state_ = false;
	down_process->move_state_ = -1;
	down_process->vector = (*print_)[print_->size() - 1]->vector;

	print_->push_back(down_process);

}

void ProcessAnalyzer::Write()
{



	FILE *fp = fopen("I:\\Users\\Alienware\\Desktop\\Result\\Point.txt", "w");

	FILE *fs= fopen("I:\\Users\\Alienware\\Desktop\\Result\\FanState.txt", "w");
	
	FILE *ss = fopen("I:\\Users\\Alienware\\Desktop\\Result\\SpeedState.txt","w");

	FILE *rr = fopen("I:\\Users\\Alienware\\Desktop\\Result\\Vector.txt", "w");

	if (rr==NULL)
	{
		cout << "Error: Vector.txt miss" << endl;
	}
	double thick = 0.2;


	for (int i = 0; i < 7; i++)
	{
		point p = (*print_)[5*i]->start_;
		fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
		fprintf(fp, "\n");
		p = (*print_)[5*(i+1)]->start_;
		fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
		fprintf(fp, "\n");	
		fprintf(fs, "%d", 0);
		fprintf(fs, "\n");
		fprintf(ss, "%d", 6);
		fprintf(ss, "\n");
		point v = point(0, 0, 1);
		fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
		fprintf(rr, "\n");
		fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
		fprintf(rr, "\n");
	}


	point p = (*print_)[5*7]->start_;
	fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
	fprintf(fp, "\n");
	p = (*print_)[0]->start_;
	fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
	fprintf(fp, "\n");
	fprintf(fs, "%d", 0);
	fprintf(fs, "\n");
	fprintf(ss, "%d", 6);
	fprintf(ss, "\n");
	point v = point(0, 0, 1);
	fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
	fprintf(rr, "\n");
	fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
	fprintf(rr, "\n");



	for (int i = 0; i <print_->size(); i++)
	{

		/*if ((*print_)[i]->start_.z() < (*print_)[i]->end_.z())
		{
			(*print_)[i]->start_.z() += thick;
			(*print_)[i]->end_.z() -= thick;
		}
		else if ((*print_)[i]->start_.z() > (*print_)[i]->end_.z())
		{
			(*print_)[i]->start_.z() -= thick;
			(*print_)[i]->end_.z() += thick;
		}*/

		
		point p = (*print_)[i]->start_;
		fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
		fprintf(fp, "\n");

		 p = (*print_)[i]->end_;
		fprintf(fp, "%lf ,%lf ,%lf", p.x(), p.y(), p.z());
		fprintf(fp, "\n");



		bool e = (*print_)[i]->fan_state_;
		if (i >= 87)
			e = true;
		fprintf(fs, "%d", e);
		fprintf(fs, "\n");

		int m = (*print_)[i]->move_state_;
		fprintf(ss, "%d", m);
		fprintf(ss, "\n");



		point v = (*print_)[i]->vector;

		fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
		fprintf(rr, "\n");

		fprintf(rr, "%lf, %lf, %lf", v.x(), v.y(), v.z());
		fprintf(rr, "\n");

	}

	std::fclose(fp);
	std::fclose(fs);
	std::fclose(ss);
	std::fclose(rr);
}

void ProcessAnalyzer::SetThick()
{
	double thick = 0.3;// In GH, it will be 0.2*5=1mm;
	
	for (int i = 0; i < print_->size(); i++)
	{
		Process* temp = (*print_)[i];

		if (temp->move_state_ == 0)
		{
			(*print_)[i + 3]->end_ += point(0, 0, thick);
			(*print_)[i + 4]->start_ += point(0, 0, thick);
		}

		if (temp->move_state_ == 4)
		{
			(*print_)[i]->start_ += point(0, 0, thick);
			(*print_)[i]->end_ += point(0, 0,thick);
			(*print_)[i]->fan_state_ = 1;
		}





		if (temp->move_state_ == 7)
		{
			temp->move_state_ == 4;
			(*print_)[i]->start_ += point(0, 0, thick);
			(*print_)[i]->end_ += point(0, 0, thick);
			(*print_)[i]->fan_state_ = 1;
		}

		//Special
		if (temp->move_state_ == 2)
		{
			(*print_)[i]->fan_state_ = 1;
			(*print_)[i]->start_ += point(0, 0, thick);
			(*print_)[i]->end_ += point(0, 0, thick);

		}
		
		if (temp->move_state_ == 3)
		{
			temp->move_state_ == 4;
			(*print_)[i]->fan_state_ = 1;
			(*print_)[i]->start_ += point(0, 0, thick);
			(*print_)[i]->end_ += point(0, 0, thick);
		}

	}


	
}
