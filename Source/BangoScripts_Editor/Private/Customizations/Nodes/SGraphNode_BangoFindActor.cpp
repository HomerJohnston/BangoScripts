#include "SGraphNode_BangoFindActor.h"

#include "GraphEditorSettings.h"
#include "IDocumentation.h"
#include "SCommentBubble.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "TutorialMetaData.h"
#include "BangoScripts/Components/BangoActorIDComponent.h"
#include "BangoScripts/Utility/BangoUtility.h"
#include "BangoScripts/EditorTooling/BangoColors.h"
#include "BangoScripts/Uncooked/K2Nodes/K2Node_BangoFindActor.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "BangoScripts"

void SGraphNode_BangoFindActor::Construct(const FArguments& InArgs, class UEdGraphNode* InNode)
{
	GraphNode = InNode;
	
	UpdateGraphNode();
	
	/*
	if (LeftNodeBox->NumSlots() == 0)
	{
		LeftNodeBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(8, 6, 4, 4)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("FindActorNode_IDLabel", "ID: {0}"), FText::FromName(GetBangoFindActorNode()->GetTargetActorID())))
		];
	}
	*/
}

TSharedRef<SWidget> SGraphNode_BangoFindActor::CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle)
{
	TSharedRef<SWidget> Text = SNew(STextBlock)
		.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitle" )
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OverflowPolicy(GetNameOverflowPolicy());

	TSubclassOf<AActor> Subclass = GetBangoFindActorNode()->GetCastTo();
	
	TSharedPtr<SImage> ClassIcon = SNew(SImage)
	.Image(FSlateIconFinder::FindIconBrushForClass(Subclass ? Subclass : NULL))
	.ColorAndOpacity(this, &SGraphNode_BangoFindActor::ColorAndOpacity_ActorLabel);
	
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

void SGraphNode_BangoFindActor::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (!GetBangoFindActorNode()->GetTargetActor().IsNull())
	{
		FString LevelPackageName = FPackageName::GetShortName(GetBangoFindActorNode()->GetTargetActor().GetLongPackageName());
		
		MainBox->AddSlot()
		.Padding(8, 4)
		[
			SNew(STextBlock)
			.WrapTextAt(128.0f)
			.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitleExtraLines" )
			.Text(FText::FromString(LevelPackageName))
		];
	}
}

void SGraphNode_BangoFindActor::CreateBelowWidgetControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (!GetBangoFindActorNode()->GetTargetActor().IsNull())
	{
		FString LevelPackageName = FPackageName::GetShortName(GetBangoFindActorNode()->GetTargetActor().GetLongPackageName());
		
		MainBox->AddSlot()
		.Padding(8, 4)
		[
			SNew(STextBlock)
			.WrapTextAt(128.0f)
			.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitleExtraLines" )
			.Text(FText::FromString(LevelPackageName))
		];
	}
}

void SGraphNode_BangoFindActor::CreatePinWidgets()
{
	SGraphNodeK2Base::CreatePinWidgets();
}

TSharedPtr<SGraphPin> SGraphNode_BangoFindActor::CreatePinWidget(UEdGraphPin* Pin) const
{
	TSharedPtr<SGraphPin> PinWidget = SGraphNodeK2Base::CreatePinWidget(Pin);
	
	PinWidget->SetShowLabel(false);
	
	return PinWidget;
}

void SGraphNode_BangoFindActor::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	SGraphNodeK2Base::AddPin(PinToAdd);
}

TArray<FOverlayWidgetInfo> SGraphNode_BangoFindActor::GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets = SGraphNode::GetOverlayWidgets(bSelected, WidgetSize);
	
	TSoftObjectPtr<AActor> TargetActor = GetTargetActorSoft();
	
	if (!TargetActor.IsNull())
	{
		FOverlayWidgetInfo& HashColorInfo = Widgets.Add_GetRef(FOverlayWidgetInfo());
	
		uint32 Hash = GetTypeHash(TargetActor);
		
		const float Size = 12.0f;
		const float HPadding = 10.0f;
		const float VPadding = 6.0f;
		
		HashColorInfo.Widget = SNew(SImage)
		.Image(FAppStyle::Get().GetBrush("Icons.FilledCircle"))
		.DesiredSizeOverride(FVector2D(Size))
		.ColorAndOpacity(Bango::Colors::Funcs::GetHashedColor(Hash, 1.0f, 1.0f));
	
		HashColorInfo.OverlayOffset = FVector2f(HPadding, WidgetSize.Y - Size - VPadding);
	}
	
#if 0
	if (!GetBangoFindActorNode()->GetTargetActor().IsNull())
	{
		return Widgets;
	}
	
	TSubclassOf<AActor> Subclass = GetBangoFindActorNode()->GetCastTo();
	
	if (Subclass)
	{
		FString Label = Subclass->GetName();
		
		const int32 MaxChars = 15;
		const FString Ellipsis = "...";
		
		if (Label.Len() > MaxChars + 3)
		Label = Label.Left(MaxChars) + Ellipsis;
		
		FOverlayWidgetInfo& CastInfo = Widgets.Add_GetRef(FOverlayWidgetInfo());
		
		CastInfo.Widget = SNew(SBorder)
		.BorderImage(nullptr)
		.Padding(0)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.ViaCast")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			]
		];
		
		CastInfo.OverlayOffset = FVector2f((WidgetSize.X + 6), 14);
	}
#endif
	
	return Widgets;
}

UK2Node_BangoFindActor* SGraphNode_BangoFindActor::GetBangoFindActorNode() const
{
	return Cast<UK2Node_BangoFindActor>(GetNodeObj());
}

void SGraphNode_BangoFindActor::UpdateGraphNode()
{
	UK2Node* K2Node = CastChecked<UK2Node>(GraphNode);
	const bool bCompactMode = K2Node->ShouldDrawCompact();

	if (bCompactMode)	
	{
		UpdateCompactNode();
	}
	else
	{
		UpdateStandardNode();
	}
}

void SGraphNode_BangoFindActor::UpdateCompactNode()
{
	
	InputPins.Empty();
	OutputPins.Empty();

	// error handling set-up
	SetupErrorReporting();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

 	TSharedPtr<SToolTip> NodeToolTip = SNew( SToolTip );
	if (!GraphNode->GetTooltipText().IsEmpty())
	{
		NodeToolTip = IDocumentation::Get()->CreateToolTip( TAttribute< FText >( this, &SGraphNode::GetNodeTooltip ), NULL, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName() );
	}

	// Setup a meta tag for this node
	FGraphNodeMetaData TagMeta(TEXT("Graphnode"));
	PopulateMetaTag(&TagMeta);
	
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode)
		.Text(this, &SGraphNode_BangoFindActor::GetNodeCompactTitle);

	TSharedRef<SOverlay> NodeOverlay = SNew(SOverlay);

	TSharedRef<SWidget> TitleText = SNew(STextBlock)//AssignNew(InlineEditableText, SInlineEditableTextBlock)
		.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitle" )
		.ColorAndOpacity(this, &SGraphNode_BangoFindActor::ColorAndOpacity_ActorLabel)
		.Text(this, &SGraphNode_BangoFindActor::GetNodeCompactTitle_Impl)
		.MinDesiredWidth(20.0f)
		.OverflowPolicy(ETextOverflowPolicy::Ellipsis);
	//InlineEditableText->SetColorAndOpacity(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateSP(this, &SGraphNode::GetNodeTitleTextColor)));

	TSubclassOf<AActor> Subclass = GetBangoFindActorNode()->GetCastTo();
	
	TSharedPtr<SImage> ClassIcon = SNew(SImage)
		.Image(FSlateIconFinder::FindIconBrushForClass(Subclass ? Subclass : NULL))
		.ColorAndOpacity(this, &SGraphNode_BangoFindActor::ColorAndOpacity_ActorLabel);
	
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	
	if (ClassIcon)
	{
		Box->AddSlot()
		.AutoWidth()
		.Padding(0, 4, 4, 0)
		.VAlign(VAlign_Top)
		[
			ClassIcon.ToSharedRef()
		];
	}
	
	FString LevelPackageName = FPackageName::GetShortName(GetBangoFindActorNode()->GetTargetActor().GetLongPackageName());
	
	Box->AddSlot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4, 0, 2)
		[
			SNew(SBox)
			.HeightOverride(16)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.MaxDesiredWidth(160)
					[
						TitleText
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(STextBlock)
			.WrapTextAt(128.0f)
			.TextStyle( FAppStyle::Get(), "Graph.Node.NodeTitleExtraLines" )
			.Text(FText::Format(LOCTEXT("FindActorNode_MapLabel", "from {0}"), FText::FromString(LevelPackageName)))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(STextBlock)
			.Visibility(this, &SGraphNode_BangoFindActor::Visibility_UnloadedIndicator)
			.WrapTextAt(128.0f)
			.TextStyle(FAppStyle::Get(), "Menu.Heading")
			//.Text(this, &SGraphNode_BangoFindActor::Text_GuidStatusWidget)
			.Text(LOCTEXT("Text_GuidStatusWidget_UnloadedActor", "UNLOADED"))
		]
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

FText SGraphNode_BangoFindActor::GetNodeCompactTitle_Impl() const
{
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	check(Node);
	
	return Node->GetNodeTitle(ENodeTitleType::Type::FullTitle);
}

FSlateColor SGraphNode_BangoFindActor::ColorAndOpacity_ActorLabel() const
{
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	check(Node);
	
	if (Node->GetTargetActor().IsPending())
	{
		return Bango::Colors::LightOrange;
	}
	
	if (Node->GetErrorState() != EBangoFindActorNode_ErrorState::OK)
	{
		return Bango::Colors::OrangeRed;
	}
	
	return Bango::Colors::White;
}

EVisibility SGraphNode_BangoFindActor::Visibility_BangoNameIndicator() const
{
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	TSoftObjectPtr<AActor> ActorSoft = Node->GetTargetActor();
						
	if (ActorSoft.IsNull())
	{
		return EVisibility::Collapsed;
	}
	else if (ActorSoft.IsPending())
	{
		return EVisibility::Collapsed;
	}
						
	return EVisibility::Visible;
}

EVisibility SGraphNode_BangoFindActor::Visibility_UnloadedIndicator() const
{
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	TSoftObjectPtr<AActor> ActorSoft = Node->GetTargetActor();
						
	if (ActorSoft.IsPending())
	{
		return EVisibility::Visible;
	}
	
	return EVisibility::Collapsed;
}

FText SGraphNode_BangoFindActor::Text_BangoNameIndicator() const
{
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	TSoftObjectPtr<AActor> ActorSoft = Node->GetTargetActor();
						
	if (ActorSoft.IsNull())
	{
		return INVTEXT("NULL ACTOR");
	}
	else if (ActorSoft.IsPending())
	{
		return INVTEXT("UNLOADED");
	}
	else
	{
		UBangoActorIDComponent* IDComponent = Bango::Utilities::GetActorIDComponent(ActorSoft.Get());
							
		if (IDComponent)
		{
			FName ID = IDComponent->GetBangoName();
								
			if (ID != NAME_None)
			{
				return FText::Format(LOCTEXT("FindActorNode_IDLabel", "{0}"), FText::FromName(ID));									
			}
			else
			{
				return INVTEXT("(Auto ID)");
			}
		}
		else
		{
			return INVTEXT("MISSING ID COMPONENT");
		}
	}
}

void SGraphNode_BangoFindActor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNodeK2Base::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	UK2Node_BangoFindActor* Node = GetBangoFindActorNode();
	
	TSharedPtr<SGraphPanel> OwnerPanel = OwnerGraphPanelPtr.Pin();
	
	if (OwnerPanel.IsValid() && OwnerPanel->SelectionManager.IsNodeSelected(GraphNode))
	{
		Node->LastSelectedFrame = GFrameCounter;
	}
}

const TSoftObjectPtr<AActor> SGraphNode_BangoFindActor::GetTargetActorSoft() const
{
	return GetBangoFindActorNode()->GetTargetActor();
}

#undef LOCTEXT_NAMESPACE
