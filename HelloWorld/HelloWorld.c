#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiPackageList.h>

#define M_PI 3.14159265358979323846
extern void SinCos(double AngleInRadians, double *pSinAns, double *pCosAns);
int _fltused;


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS efiStatus;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    double sin, cos;

    EFI_HII_DATABASE_PROTOCOL *HiiDatabase;
    EFI_HII_IMAGE_PROTOCOL *HiiImage;
    EFI_HII_PACKAGE_LIST_HEADER *PackageList;
    EFI_HII_HANDLE HiiHandle;
    EFI_IMAGE_INPUT Image;
    EFI_PHYSICAL_ADDRESS Buffer;

    efiStatus = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID**)&HiiDatabase);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to locate HII Database\n");
        return EFI_NOT_STARTED;
    }

    efiStatus = gBS->LocateProtocol(&gEfiHiiImageProtocolGuid, NULL, (VOID**)&HiiImage);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to locate HII Image\n");
        return EFI_NOT_STARTED;
    }

    efiStatus = gBS->OpenProtocol(ImageHandle, &gEfiHiiPackageListProtocolGuid,
                  (VOID**)&PackageList, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR (efiStatus)) {
        Print(L"HII Image Package not found in PE/COFF resource section\n");
        return efiStatus;
    }

    efiStatus = HiiDatabase->NewPackageList(HiiDatabase, PackageList, NULL, &HiiHandle);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to register HII Package\n");
        return EFI_NOT_STARTED;
    }

    efiStatus = gBS->LocateProtocol(&gopGuid, NULL, (void**)&gop);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to locate GOP\n");
        return EFI_NOT_STARTED;
    }
    UINT32 *video = (UINT32*)(UINTN)gop->Mode->FrameBufferBase;

    efiStatus = HiiImage->GetImage(HiiImage, HiiHandle, IMAGE_TOKEN(IMG_LOGO), &Image);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to locate IMG_LOGO\n");
        return EFI_NOT_STARTED;
    }

    UINTN size = Image.Height * Image.Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    efiStatus = gBS->AllocatePages(AllocateAnyPages,
                  EfiLoaderData, EFI_SIZE_TO_PAGES(size), &Buffer);
    if (EFI_ERROR(efiStatus)) {
        Print(L"Unable to allocate Buffer\n");
        return EFI_NOT_STARTED;
    }

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *logo = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)(UINTN)Buffer;
    CopyMem(logo, Image.Bitmap, size);
    UINTN logo_offset = 38;

    gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    double tree_height_factor = gop->Mode->Info->VerticalResolution * .8;
    double tree_width_factor = gop->Mode->Info->VerticalResolution * .5;
    double tree_width_base = gop->Mode->Info->HorizontalResolution * .5;

    for (UINTN tick = 0; ; tick++) {
        gop->Blt(gop, logo, EfiBltVideoFill, 0, 0,
            (UINTN)(tree_width_base - 1.2 * tree_width_factor), Image.Height,
            (UINTN)(2.4 * tree_width_factor),
            gop->Mode->Info->VerticalResolution - Image.Height, 0);

        for (double t = 0; t < 1; t += .001) {
            double width = t * tree_width_factor;
            double w = 2 * M_PI * 5;
            double p = 2 * M_PI * tick / 36; // (0.5 revs / sec)
            double angle = w * t + p;
            SinCos(angle, &sin, &cos);
            double x = width * cos + tree_width_base;
            double z = width * sin;
            double y = t * tree_height_factor;
            UINTN x_disp = (UINTN)(x + z * .5);
            UINTN y_disp = (UINTN)(y + z * .25) + Image.Height;
            if (y_disp < gop->Mode->Info->VerticalResolution) {
                UINTN coord = gop->Mode->Info->PixelsPerScanLine * y_disp + x_disp;
                double color = t * 50;
                color -= (INTN)color;
                UINT32 pixel = (UINT32)(color * 255) << 8;
                pixel |= (UINT32)((1. - color) * 255) << 16;
                video[coord] = pixel;
                video[coord - 1] = pixel;
                video[coord + 1] = pixel;
                if (y_disp > 0)
                    video[coord - gop->Mode->Info->PixelsPerScanLine] = pixel;
                if (y_disp < gop->Mode->Info->VerticalResolution - 1)
                    video[coord + gop->Mode->Info->PixelsPerScanLine] = pixel;
            }
        }

        for (UINTN i = 0; i < Image.Width * Image.Height; i++) {
            if (Image.Bitmap[i].Blue > Image.Bitmap[i].Red) {
                if (sin > 0) {
                    // blend with (0,0,0)
                    logo[i].Red = (UINT8)(Image.Bitmap[i].Red * (1 - sin));
                    logo[i].Green = (UINT8)(Image.Bitmap[i].Green * (1 - sin));
                    logo[i].Blue = (UINT8)(Image.Bitmap[i].Blue * (1 - sin));
                }
                else {
                    // blend with (175, 224, 250)
                    logo[i].Red = (UINT8)(175 - (175 - Image.Bitmap[i].Red) * (1 + sin));
                    logo[i].Green = (UINT8)(224 - (224 - Image.Bitmap[i].Green) * (1 + sin));
                    logo[i].Blue = (UINT8)(250 - (250 - Image.Bitmap[i].Blue) * (1 + sin));
                }
            }
        }
        gop->Blt(gop, logo, EfiBltBufferToVideo,
            0, 0, (UINTN)tree_width_base - logo_offset, 0,
            Image.Width, Image.Height, Image.Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

        gBS->Stall((UINTN)(65536 / (105. / 88.))); // ~55 ms, to match IBM PC timer

        EFI_INPUT_KEY Key;
        efiStatus = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
        if (!EFI_ERROR(efiStatus)) {
            return efiStatus;
        }
    }
}
