// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Player/UIInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Player/BashPlayerController.h"
#include "Widgets/Menu.h"
#include "Widgets/SelectableWidget.h"
#include "Framework/Application/SlateApplication.h"

// Sets default values for this component's properties
UUIInputComponent::UUIInputComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UUIInputComponent::SetupInput(ABashPlayerController* owner)
{
	OwningPlayer = owner;

	if (OwningPlayer)
	{
		ULocalPlayer* localOwningPlayer = OwningPlayer->GetLocalPlayer();

		if (UEnhancedInputLocalPlayerSubsystem* inputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localOwningPlayer))
		{
			inputSubsystem->AddMappingContext(MenuMappingContext, 1);

			if (UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(OwningPlayer->InputComponent))
			{
				enhancedInputComponent->BindAction(UpNavigateAction, ETriggerEvent::Started, this, &UUIInputComponent::NavigateUp);
				enhancedInputComponent->BindAction(DownNavigateAction, ETriggerEvent::Started, this, &UUIInputComponent::NavigateDown);
				enhancedInputComponent->BindAction(LeftNavigateAction, ETriggerEvent::Started, this, &UUIInputComponent::NavigateLeft);
				enhancedInputComponent->BindAction(RightNavigateAction, ETriggerEvent::Started, this, &UUIInputComponent::NavigateRight);

				enhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &UUIInputComponent::SelectOption);
				enhancedInputComponent->BindAction(BackAction, ETriggerEvent::Started, this, &UUIInputComponent::BackButtonPressed);
				enhancedInputComponent->BindAction(StartAction, ETriggerEvent::Started, this, &UUIInputComponent::StartButtonPressed);
				
				UE_LOG(LogTemp, Log, TEXT("UI Input bound"));
			}
		}
	}
}

// Called when the game starts
void UUIInputComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UUIInputComponent::FocusWidget(UUserWidget* widget)
{
	if (widget && widget->bIsFocusable)
	{
		FocusedWidget = widget;

		//Update last selected widget on stack.
		if (MenuStack.Top())
		{
			MenuStack.Top()->SetSelectedWidget(widget);
		}

		widget->SetUserFocus(OwningPlayer);
		//GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("%s focused!"), *widget->GetName()));
	}
}
	
void UUIInputComponent::NavigateUp()
{
	OnUpButtonPressed.Broadcast(OwningPlayer);
	NavigateDirection(ENavigationDirection::END_Up);
}

void UUIInputComponent::NavigateDown()
{
	OnDownButtonPressed.Broadcast(OwningPlayer);
	NavigateDirection(ENavigationDirection::END_Down);
}

void UUIInputComponent::NavigateLeft()
{
	OnLeftButtonPressed.Broadcast(OwningPlayer);
	NavigateDirection(ENavigationDirection::END_Left);
}

void UUIInputComponent::NavigateRight()
{
	OnRightButtonPressed.Broadcast(OwningPlayer);
	NavigateDirection(ENavigationDirection::END_Right);
}


void UUIInputComponent::NavigateDirection(ENavigationDirection direction)
{
	if (FocusedWidget && FocusedWidget->Navigation)
	{
		
		TWeakObjectPtr<UUserWidget> nextWidget;

		switch (direction)
		{
			case ENavigationDirection::END_Up:
				nextWidget = Cast<UUserWidget>(FocusedWidget->Navigation->Up.Widget);
			break;
			case ENavigationDirection::END_Down:
				nextWidget = Cast<UUserWidget>(FocusedWidget->Navigation->Down.Widget);
			break;
			case ENavigationDirection::END_Left:
				nextWidget = Cast<UUserWidget>(FocusedWidget->Navigation->Left.Widget);
			break;
			case ENavigationDirection::END_Right:
				nextWidget = Cast<UUserWidget>(FocusedWidget->Navigation->Right.Widget);
			break;
			default: nextWidget = nullptr;
		}
		
		if (nextWidget.IsValid())
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("try focus %s" ), *nextWidget->GetName()));

			FocusWidget(&(*nextWidget));
		}
	}
}


void UUIInputComponent::SelectOption()
{

	bool inputConsumed = false;
	
	if (FocusedWidget)
	{
		
		//GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, FString::Printf(TEXT("Selected %s"), *FocusedWidget->GetName()));
		if (FocusedWidget->GetClass()->ImplementsInterface(USelectableWidget::StaticClass()))
		{
			inputConsumed = true;
			ISelectableWidget::Execute_OnSelected(FocusedWidget);
		}else
		{
			UButton* Button = Cast<UButton>(FocusedWidget);
			if (Button)
			{
				Button->OnClicked.Broadcast();
				inputConsumed = true;
			}
		}

	}

	//only broadcast event if input is not consumed by activating button/selectable widget.
	if (!inputConsumed)
	{
		OnSelectButtonPressed.Broadcast(OwningPlayer);
	}
}

void UUIInputComponent::StartButtonPressed()
{
	OnStartButtonPressed.Broadcast(OwningPlayer);
}

void UUIInputComponent::BackButtonPressed()
{
	OnBackButtonPressed.Broadcast(OwningPlayer);
}


void UUIInputComponent::OpenMenu(UMenu* menu, bool clearMenuStack)
{
	//create new menu context
	UMenuStackContext* newMenuContext = NewObject<UMenuStackContext>();
	newMenuContext->SetMenu(menu);
	newMenuContext->SetSelectedWidget(menu->GetDefaultSelectedWidget());

	if (!MenuStack.IsEmpty())
	{
		//Exit Previous Menu
		UMenuStackContext* oldMenuContext = MenuStack.Top();
		if (oldMenuContext->GetMenu())
		{
			oldMenuContext->GetMenu()->OnExitMenu(OwningPlayer);
		}
		
		//Close all menus and clear stack
		if (clearMenuStack)
		{
			while (!MenuStack.IsEmpty())
			{
				UMenuStackContext* menuContext = MenuStack.Pop();

				if (menuContext)
				{
					if (menuContext->GetMenu())
						menuContext->GetMenu()->OnCloseMenu();
				}
			}
			
			MenuStack.Empty();
		}
	}
	
	MenuStack.Push(newMenuContext);
	
	menu->OnOpenMenu(OwningPlayer);
	menu->OnEnterMenu(OwningPlayer);
	FocusWidget(newMenuContext->GetSelectedWidget());
}

void UUIInputComponent::ClearWidgetFocus()
{
	if (ULocalPlayer* localPlayer = OwningPlayer->GetLocalPlayer())
	{
		int id = FSlateApplication::Get().GetUserIndexForController(localPlayer->GetControllerId());
		FSlateApplication::Get().SetUserFocusToGameViewport(id);
		FocusedWidget = nullptr;
	}
}

void UUIInputComponent::CloseMenu(UMenu* menu, bool closeAll)
{
	UMenuStackContext* checkContext = NewObject<UMenuStackContext>();
	checkContext->SetMenu(menu);
	
	if (MenuStack.ContainsByPredicate([&](UMenuStackContext* menuContext)
	{
		return (menuContext->GetMenu() == menu);
	}))
	{

		UMenuStackContext* curMenuContext;

		//close each menu from the top until we reach desired menu.
		do
		{
			if (MenuStack.IsEmpty())break;
			
			curMenuContext = MenuStack.Pop();

			if (curMenuContext->GetMenu())
			{
				curMenuContext->GetMenu()->OnExitMenu(OwningPlayer);
				curMenuContext->GetMenu()->OnCloseMenu();
			}
		}while (curMenuContext && (closeAll || curMenuContext->GetMenu() != menu));

		//set new top to active menu.
		if (!MenuStack.IsEmpty())
		{
			curMenuContext = MenuStack.Top();
			if (curMenuContext && curMenuContext->GetSelectedWidget())
			{
				curMenuContext->GetMenu()->OnEnterMenu(OwningPlayer);
				FocusWidget(curMenuContext->GetSelectedWidget());
			}
		}else
		{
			//TODO: if no top, clear widget focus

			ClearWidgetFocus();
		}
	}else
	{
		UE_LOG(LogTemp, Error, TEXT("Menu stack does not contain menu requested to close."));
	}
}

void UUIInputComponent::CloseAllMenus()
{
	if (!MenuStack.IsEmpty())
	{
		CloseMenu(MenuStack.Top()->GetMenu(), true);
	}
}


//UMenuContext Definitions

void UMenuStackContext::SetMenu(UMenu* menu)
{
	Menu = MakeWeakObjectPtr(menu);
}
UMenu* UMenuStackContext::GetMenu() const
{
	return Menu.Get();
}
void UMenuStackContext::SetSelectedWidget(UUserWidget* widget)
{
	SelectedWidget = MakeWeakObjectPtr(widget);
}
UUserWidget* UMenuStackContext::GetSelectedWidget() const
{
	return SelectedWidget.Get();
}

