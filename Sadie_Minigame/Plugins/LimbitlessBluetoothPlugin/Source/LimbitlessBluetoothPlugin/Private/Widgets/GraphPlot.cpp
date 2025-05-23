// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/GraphPlot.h"

void UGraphPlot::SetData(const TArray<float>& InData)
{
	if (bAutoscaleToData)
	{
		SetGraphScaled(InData);
	}
	else
	{
		SetGraphUnscaled(InData);
	}
}


void UGraphPlot::SetGraphScaled(const TArray<float>& InData)
{
	if (InData.Num() <= 0) { return; }
	
	float Min = InData[0];
	float Max = InData[0];
	
	for (const auto Point : InData)
	{
		if (Point < Min)
		{
			Min = Point;
		}
		else if (Point > Max)
		{
			Max = Point;
		}
	}

	GraphHalfSize = FMath::Max(Max, -Min);
	GraphHalfSize = FMath::Max(GraphHalfSize, MinGraphSize);
	
	FVector2D LastSize = GetCachedGeometry().GetLocalSize();
	GraphPoints.Reset();

	for (int i = 0; i < InData.Num(); ++i)
	{
		// Newer values should be positioned to the right
		float PointX = (static_cast<float>(i) / InData.Num()) * LastSize.X;

		// Scale y-value to be in the graph, relative to the center.
		// Negate since positive y is down in widgets, which is contrary to graph convention
		float PointY = ((-InData[i] + GraphHalfSize) / (2 * GraphHalfSize)) * LastSize.Y;

		GraphPoints.Add({PointX, PointY });
	}
}

void UGraphPlot::SetGraphUnscaled(const TArray<float>& InData)
{
	FVector2D LastSize = GetCachedGeometry().GetLocalSize();
	GraphPoints.Reset();

	const float GraphWidth = UnscaledGraphMaximum - UnscaledGraphMinimum;
	
	for (int i = 0; i < InData.Num(); ++i)
	{
		float PointX = (static_cast<float>(i) / InData.Num()) * LastSize.X;

		//double NormalizedValue = (InData[i] + UnscaledGraphMinimum) / (GraphWidth);
		double NormalizedValue = (InData[i] - UnscaledGraphMinimum) / GraphWidth;
		NormalizedValue = FMath::Clamp(NormalizedValue, 0.0, 1.0);
		
		float PointY = (-NormalizedValue + 1) * LastSize.Y;

		GraphPoints.Add({PointX, PointY });
	}
}



int32 UGraphPlot::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                              const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                              const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	DrawGrid(OutDrawElements, LayerId, AllottedGeometry);
	++LayerId;

	DrawLines(GraphPoints, OutDrawElements, LayerId, AllottedGeometry);
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);

}

void UGraphPlot::DrawGrid(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry) const
{
	FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	if (NumHorizontalLines > 0)
	{
		float HorizontalLineGap = WidgetSize.Y / NumHorizontalLines;
		for (int i = 0; i < NumHorizontalLines; ++i)
		{
			float YPos = ((i + 1) * HorizontalLineGap);
			DrawLine(FVector2f(0, YPos), FVector2f(WidgetSize.X, YPos), OutDrawElements, LayerId, AllottedGeometry);
		}
	}
	
	if (NumVerticalLines > 0)
	{
		float VerticalLineGap = WidgetSize.X / NumVerticalLines;
		for (int j = 0; j < NumVerticalLines; ++j)
		{
			float XPos = j * VerticalLineGap + (VerticalLineGap / 2);
			DrawLine(FVector2f(XPos, 0), FVector2f(XPos, WidgetSize.Y), OutDrawElements, LayerId, AllottedGeometry);
		}
	}
}

void UGraphPlot::DrawLine(FVector2f PositionA, FVector2f PositionB, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry) const
{
	TArray<FVector2f> Points;
	Points.Add(PositionA);
	Points.Add(PositionB);

	if ((PositionA - PositionB).SquaredLength() > KINDA_SMALL_NUMBER)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			GridLineColor,
			false,
			GridLineThickness);
	}
}

void UGraphPlot::DrawLines(const TArray<FVector2f>& Points, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry) const
{
	if (Points.Num() < 2)
	{
		return;
	}

	// We need to trim points that might be overlapping. We convert to Float at the same time
	TArray<FVector2f> ValidatedPoints;
	ValidatedPoints.Reserve(Points.Num());
	ValidatedPoints.Push(Points[0]);
	FVector2f LastPoint = Points[0];
	for (int32 Index = 1; Index < Points.Num(); Index++)
	{
		FVector2f CurrentPoint = Points[Index];
		// If a the distance between two point is very small, do not add it.
		if ((CurrentPoint - LastPoint).SquaredLength() > KINDA_SMALL_NUMBER)
		{
			ValidatedPoints.Push(Points[Index]);
			LastPoint = CurrentPoint;
		}
	}
	if (ValidatedPoints.Num() < 2)
	{
		return;
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		MoveTemp(ValidatedPoints),
		ESlateDrawEffect::None,
		SampleLineColor,
		false,
		SampleLineThickness);
}
