#include "Lasso/LassoTargetComponent.h"
#include "GameFramework/Actor.h"

ULassoTargetComponent::ULassoTargetComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
    // Local X points toward the horse's head. These defaults remain editable on a horse Blueprint.
    Volumes = {
        {TEXT("Torso"),ELassoHitZone::Torso,FVector(0,0,38),4.f},
        {TEXT("Chest"),ELassoHitZone::Torso,FVector(70,0,52),4.f},
        {TEXT("Neck"),ELassoHitZone::Neck,FVector(105,0,72),4.f},
        {TEXT("Head"),ELassoHitZone::Head,FVector(175,0,105),4.f}
    };
}

FVector ULassoTargetComponent::GetVolumeCenter(const FLassoTargetVolume& Volume) const
{
    const AActor* Owner=GetOwner();
    return Owner?Owner->GetActorTransform().TransformPosition(Volume.LocalCenter):Volume.LocalCenter;
}

bool ULassoTargetComponent::FindLoopIntersection(const FVector& PreviousCenter, const FVector& NextCenter,
    const FVector& PlaneNormal, float InLoopRadius, float PlaneThickness,
    float& OutAlong, FVector& OutLocation, ELassoHitZone& OutZone) const
{
    if (!GetOwner()) { return false; }
    const FVector Normal=PlaneNormal.GetSafeNormal(KINDA_SMALL_NUMBER,FVector::ForwardVector);
    const FVector Segment=NextCenter-PreviousCenter;
    const float SegmentLengthSquared=Segment.SizeSquared();
    bool bFound=false;
    OutAlong=BIG_NUMBER;
    for (const FLassoTargetVolume& Volume : Volumes)
    {
        if (Volume.Zone==ELassoHitZone::None || Volume.Radius<=0.f) { continue; }
        const FVector Sample=GetVolumeCenter(Volume);
        const float Along=SegmentLengthSquared>SMALL_NUMBER
            ?FMath::Clamp(FVector::DotProduct(Sample-PreviousCenter,Segment)/SegmentLengthSquared,0.f,1.f):0.f;
        const FVector Center=FMath::Lerp(PreviousCenter,NextCenter,Along);
        const FVector Relative=Sample-Center;
        const float AlongNormal=FVector::DotProduct(Relative,Normal);
        const FVector InPlane=Relative-Normal*AlongNormal;
        if (FMath::Abs(AlongNormal)<=PlaneThickness+Volume.Radius
            && InPlane.SizeSquared()<=FMath::Square(InLoopRadius+Volume.Radius)
            && Along<OutAlong)
        {
            bFound=true;
            OutAlong=Along;
            OutLocation=Sample;
            OutZone=Volume.Zone;
        }
    }
    return bFound;
}

ELassoHitZone ULassoTargetComponent::ClassifyLocation(const FVector& WorldLocation) const
{
    ELassoHitZone Closest=ELassoHitZone::None;
    float BestDistanceSquared=BIG_NUMBER;
    for (const FLassoTargetVolume& Volume : Volumes)
    {
        if (Volume.Zone==ELassoHitZone::None) { continue; }
        const float DistanceSquared=FVector::DistSquared(WorldLocation,GetVolumeCenter(Volume));
        if (DistanceSquared<BestDistanceSquared)
        {
            BestDistanceSquared=DistanceSquared;
            Closest=Volume.Zone;
        }
    }
    return Closest;
}
