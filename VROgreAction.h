#ifndef _VROGREACTION_H_
#define _VROGREACTION_H_

#include "VRAction.h"

class VROChangeTSR : public VRWandAction
{
	public:
		VROChangeTSR() : VRWandAction("change_tsr") {}
		virtual ~VROChangeTSR() {}

		virtual void ButtonUp(void);

	protected:
};

class VROClearSelect : public VRWandAction
{
	public:
		VROClearSelect() : VRWandAction("clear_select") {}
		virtual ~VROClearSelect() {}

	protected:
		
};

class VRODelete : public VRWandAction
{
	public:
		VRODelete() : VRWandAction("delete") {}
		virtual ~VRODelete() {}

	protected:
		virtual void	ButtonUp(void);
};

class VRODuplicate : public VRWandAction
{
	public:
		VRODuplicate() : VRWandAction("duplicate") {}
		virtual ~VRODuplicate() {}

	protected:
		virtual void	ButtonUp(void);
};

class VRORotate : public VRWandAction
{
	public:
		VRORotate() : VRWandAction("rotate"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoRotate(false) {}
		virtual ~VRORotate() {}

		virtual	void	ButtonDown(void);
		virtual void	ButtonUp(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	protected:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoRotate;
};

class VROSave : public VRWandAction
{
	public:
		VROSave() : VRWandAction("save") {}
		virtual ~VROSave() {}

		virtual void ButtonUp(void);

	protected:
};

class VROScale : public VRWandAction
{
	public:
		VROScale() : VRWandAction("scale"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoScale(false), m_bLarger(true) {}
		virtual ~VROScale() {}

		void SetLarger(bool bLarger) { m_bLarger = bLarger; }

		virtual	void	ButtonDown(void);
		virtual void	ButtonUp(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	protected:

	private:
		float m_fSelectionDistance;
		jvec3  m_vOffset;
		bool  m_bDoScale;
		bool  m_bLarger;
};


class VROSelect : public VRWandAction
{
	public:
		VROSelect() : VRWandAction("select"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoTranslate(false) {}
		virtual ~VROSelect() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	private:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoTranslate;
};


class VROTranslate : public VRWandAction
{
	public:
		VROTranslate() : VRWandAction("translate"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoTranslate(false) {}
		virtual ~VROTranslate() {}

		virtual	void	ButtonDown(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);
		virtual void	ButtonUp(void);

	protected:

	private:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoTranslate;
};

class VROVizSelect : public VRWandAction
{
	public:
		VROVizSelect() : VRWandAction("viz_select") {}
		virtual ~VROVizSelect() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}

	private:

		void DeselectAll(void);
};

class VROWCVizSelect : public VRWandAction
{
	public:
		VROWCVizSelect() : VRWandAction("wc_viz_select") {}
		virtual ~VROWCVizSelect() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}

	private:

		void DeselectAll(void);
};

class VROSwitchVizMat : public VRWandAction
{
	public:
		VROSwitchVizMat() : VRWandAction("switch_viz_materials") {}
		virtual ~VROSwitchVizMat() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}
};

class VROWCToggleEvents : public VRWandAction
{
	public:
		VROWCToggleEvents() : VRWandAction("wc_event_toggle") {}
		virtual ~VROWCToggleEvents() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}
};

class VROWCToggleArcs : public VRWandAction
{
	public:
		VROWCToggleArcs() : VRWandAction("wc_arc_toggle") {}
		virtual ~VROWCToggleArcs() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}
};

class VROWCSideView : public VRWandAction
{
	public:
		VROWCSideView() : VRWandAction("wc_side_view"), previousPosition(0.f, 0.f, 0.f), previousOrientation(1.f, 0.f, 0.f, 0.f) {}
		virtual ~VROWCSideView() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}
protected:
		jvec3 previousPosition;
		quat previousOrientation;
};

class VROWCFollowPath : public VRWandAction
{
	public:
		VROWCFollowPath() : VRWandAction("wc_follow_path") {}
		virtual ~VROWCFollowPath() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void) {}

};

class VROTSR : public VRWandAction
{
	public:
		VROTSR() : VRWandAction("tsr") {}

		virtual ~VROTSR() {}

	protected:

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:
		VROTranslate m_translateAction;
		VROScale m_scaleAction;
		VRORotate m_rotateAction;
};
#endif