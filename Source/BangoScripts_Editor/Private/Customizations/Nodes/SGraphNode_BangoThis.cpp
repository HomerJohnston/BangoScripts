#include "SGraphNode_BangoThis.h"

#include "GraphEditorSettings.h"
#include "SCommentBubble.h"
#include "SGraphPin.h"
#include "TutorialMetaData.h"
#include "BangoScripts/Core/BangoScriptBlueprint.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoThis.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

void SGraphNode_BangoThis::Construct(const FArguments& InArgs, class UEdGraphNode* InNode)
{
	GraphNode = InNode;
	
	UpdateGraphNode();
}

TSharedRef<SWidget> SGraphNode_BangoThis::CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle)
{
	TSharedRef<SWidget> Text = SNew(STextBlock)
		.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitle" )
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OverflowPolicy(GetNameOverflowPolicy());

	TSubclassOf<AActor> Subclass = GetBangoThisNode()->GetClassType();
	
	TSharedPtr<SImage> ClassIcon = SNew(SImage)
	.Image(FSlateIconFinder::FindIconBrushForClass(Subclass ? Subclass : NULL))
	.ColorAndOpacity(this, &SGraphNode_BangoThis::ColorAndOpacity_ActorLabel);
	
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	
	if (ClassIcon)
	{
		Box->AddSlot()
		.AutoWidth()
		.Padding(0, 0, 8, 0)
		[
			ClassIcon.ToSharedRef()
		];
	}
	
	Box->AddSlot()
	[
		Text
	];
	
	return Box;
}

void SGraphNode_BangoThis::CreatePinWidgets()
{
	SGraphNodeK2Base::CreatePinWidgets();
}

TSharedPtr<SGraphPin> SGraphNode_BangoThis::CreatePinWidget(UEdGraphPin* Pin) const
{
	TSharedPtr<SGraphPin> PinWidget = SGraphNodeK2Base::CreatePinWidget(Pin);
	
	PinWidget->SetShowLabel(false);
	
	return PinWidget;
}

void SGraphNode_BangoThis::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	SGraphNodeK2Base::AddPin(PinToAdd);
}

UK2Node_BangoThis* SGraphNode_BangoThis::GetBangoThisNode() const
{
	return Cast<UK2Node_BangoThis>(GetNodeObj());
}

void SGraphNode_BangoThis::UpdateGraphNode()
{
	UpdateCompactNode();
}

void SGraphNode_BangoThis::UpdateCompactNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// error handling set-up
	SetupErrorReporting();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	// Setup a meta tag for this node
	FGraphNodeMetaData TagMeta(TEXT("Graphnode"));
	PopulateMetaTag(&TagMeta);
	
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode)
		.Text(this, &SGraphNode_BangoThis::GetNodeCompactTitle);

	TSharedRef<SOverlay> NodeOverlay = SNew(SOverlay);

	TSharedRef<SWidget> TitleText = SNew(STextBlock)
		.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitle" )
		.ColorAndOpacity(this, &SGraphNode_BangoThis::ColorAndOpacity_NodeTitle)
		.Text(this, &SGraphNode_BangoThis::Text_NodeTitle)
		.MinDesiredWidth(20.0f)
		.OverflowPolicy(ETextOverflowPolicy::Ellipsis);

	/*
	TSharedRef<SWidget> ActorLabelText = SNew(STextBlock)
		.TextStyle(FAppStyle::Get(), "Menu.Heading")
		.ColorAndOpacity(this, &SGraphNode_BangoThis::ColorAndOpacity_ActorLabel)
		.Text(this, &SGraphNode_BangoThis::Text_ActorLabel)
		.MinDesiredWidth(20.0f)
		.OverflowPolicy(ETextOverflowPolicy::MultilineEllipsis);
	*/
	
	TSubclassOf<AActor> Subclass = GetBangoThisNode()->GetClassType();
	
	TSharedPtr<SImage> ClassIcon = SNew(SImage)
		.Image(FSlateIconFinder::FindIconBrushForClass(Subclass ? Subclass : NULL))
		.ColorAndOpacity(this, &SGraphNode_BangoThis::ColorAndOpacity_NodeTitle);
	
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	
	if (ClassIcon)
	{
		Box->AddSlot()
		.AutoWidth()
		.Padding(0, 0, 4, 0)
		.VAlign(VAlign_Top)
		[
			ClassIcon.ToSharedRef()
		];
	}
	
	Box->AddSlot()
	.VAlign(VAlign_Center)
	[
		TitleText
		/*
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4, 0, 2)
		[
			SNew(SBox)
			.HeightOverride(16)
			.MaxDesiredWidth(120)
			[
				TitleText
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(-20, 2, -40, 4)
		[
			SNew(SBox)
			.MaxDesiredWidth(120)
			[
				ActorLabelText
			]
		]
		*/
	];
	
	NodeOverlay->AddSlot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(8.f, 0.f, 45.f, 0.f)
	[
		Box
	];
	
	// Default to "pure" styling, where we can just center the pins vertically
	// since don't need to worry about alignment with other nodes
	float PinPaddingTop = 0.f;

	static float MinNodePadding = 55.f;
	// Calculate a padding amount clamping to the min/max settings
	float PinPaddingRight = MinNodePadding;

	EVerticalAlignment PinVerticalAlignment = VAlign_Top; // VAlign_Center;

	// But if this is an impure node, we'll align the pins to the top, 
	// and add some padding so that the exec pins line up with the exec pins of other nodes
	if (UK2Node* K2Node = Cast<UK2Node>(GraphNode))
	{
		const bool bIsPure = K2Node->IsNodePure();
		if (!bIsPure)
		{
			PinPaddingTop += 8.0f;
			PinVerticalAlignment = VAlign_Top;
		}

		if (K2Node->ShouldDrawCompact() && bIsPure)
		{
			// If the center node title is 2 or more, then make the node bigger
			// so that the text box isn't over top of the label
			static float MaxNodePadding = 180.0f;
			static float PaddingIncrementSize = 20.0f;

			int32 HeadTitleLength = NodeTitle.Get() ? NodeTitle.Get()->GetHeadTitle().ToString().Len() : 0;

			PinPaddingRight = FMath::Clamp(MinNodePadding + ((float)HeadTitleLength * PaddingIncrementSize), MinNodePadding, MaxNodePadding);
		}
	}

	NodeOverlay->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(PinVerticalAlignment)
		.Padding(/* left */ 0.f, PinPaddingTop, PinPaddingRight, /* bottom */ 0.f)
		[
			// LEFT
			SAssignNew(LeftNodeBox, SVerticalBox)
		];

	NodeOverlay->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(PinVerticalAlignment)
		.Padding(55.f, PinPaddingTop, 0.f, 0.f)
		[
			// RIGHT
			SAssignNew(RightNodeBox, SVerticalBox)
		];

	//
	//             ______________________
	//            | (>) L |      | R (>) |
	//            | (>) E |      | I (>) |
	//            | (>) F |   +  | G (>) |
	//            | (>) T |      | H (>) |
	//            |       |      | T (>) |
	//            |_______|______|_______|
	//
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	
	TSharedRef<SVerticalBox> InnerVerticalBox =
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			// NODE CONTENT AREA
			SNew( SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( GetStyleSet().GetBrush("Graph.VarNode.Body") )
				.ColorAndOpacity(Bango::Colors::White)
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FAppStyle::GetBrush("Graph.VarNode.ColorSpill") )
				.ColorAndOpacity(GraphNode->GetNodeTitleColor())
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image( GetStyleSet().GetBrush("Graph.VarNode.Gloss") )
			]
			+SOverlay::Slot()
			.Padding( FMargin(0,3) )
			[
				NodeOverlay
			]
		];
	
	TSharedPtr<SWidget> EnabledStateWidget = GetEnabledStateWidget();
	if (EnabledStateWidget.IsValid())
	{
		InnerVerticalBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.Padding(FMargin(2, 0))
			[
				EnabledStateWidget.ToSharedRef()
			];
	}

	InnerVerticalBox->AddSlot()
		.AutoHeight()
		.Padding( FMargin(5.0f, 1.0f) )
		[
			ErrorReporting->AsWidget()
		];

	this->GetOrAddSlot( ENodeZone::Center )
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	[
		InnerVerticalBox
	];

	CreatePinWidgets();

	// Hide pin labels
	for (auto InputPin: this->InputPins)
	{
		if (InputPin->GetPinObj()->ParentPin == nullptr)
		{
			InputPin->SetShowLabel(false);
		}
	}

	for (auto OutputPin : this->OutputPins)
	{
		if (OutputPin->GetPinObj()->ParentPin == nullptr)
		{
			OutputPin->SetShowLabel(false);
		}
	}

	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew( CommentBubble, SCommentBubble )
	.GraphNode( GraphNode )
	.Text( this, &SGraphNode::GetNodeComment )
	.OnTextCommitted( this, &SGraphNode::OnCommentTextCommitted )
	.ColorAndOpacity( CommentColor )
	.AllowPinning( true )
	.EnableTitleBarBubble( true )
	.EnableBubbleCtrls( true )
	.GraphLOD( this, &SGraphNode::GetCurrentLOD )
	.IsGraphNodeHovered( this, &SGraphNodeK2Base::IsHovered );

	GetOrAddSlot( ENodeZone::TopCenter )
	.SlotOffset2f( TAttribute<FVector2f>( CommentBubble.Get(), &SCommentBubble::GetOffset2f ))
	.SlotSize2f( TAttribute<FVector2f>( CommentBubble.Get(), &SCommentBubble::GetSize2f ))
	.AllowScaling( TAttribute<bool>( CommentBubble.Get(), &SCommentBubble::IsScalingAllowed ))
	.VAlign( VAlign_Top )
	[
		CommentBubble.ToSharedRef()
	];

	CreateInputSideAddButton(LeftNodeBox);
	CreateOutputSideAddButton(RightNodeBox);
}

FText SGraphNode_BangoThis::Text_NodeTitle() const
{
	UK2Node_BangoThis* Node = GetBangoThisNode();
	
	return Node->GetNodeTitle(ENodeTitleType::Type::FullTitle);
}

FText SGraphNode_BangoThis::Text_ActorLabel() const
{
	UK2Node_BangoThis* Node = GetBangoThisNode();

	UBangoScriptBlueprint* Blueprint = Node->GetBangoScriptBlueprint();
	
	if (Blueprint && Blueprint->GetOwnerActor().IsValid())
	{
		return FText::FromString(Blueprint->GetOwnerActor()->GetActorLabel());
	}
	else
	{
		return LOCTEXT("ThisNode_ActorLabel_Unloaded", "Unloaded");
	}
}

FSlateColor SGraphNode_BangoThis::ColorAndOpacity_NodeTitle() const
{
	return Bango::Colors::White;
}

FSlateColor SGraphNode_BangoThis::ColorAndOpacity_ActorLabel() const
{
	UK2Node_BangoThis* Node = GetBangoThisNode();

	UBangoScriptBlueprint* Blueprint = Node->GetBangoScriptBlueprint();
	
	if (Blueprint && Blueprint->GetOwnerActor().IsValid())
	{
		return Bango::Colors::Gray;
	}
	else
	{
		return Bango::Colors::Red;
	}
}

#undef LOCTEXT_NAMESPACE
