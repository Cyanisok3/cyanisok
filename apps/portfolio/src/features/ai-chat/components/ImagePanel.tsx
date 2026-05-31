import type { FormEvent } from "react";
import { ImageIcon, Loader2, UploadCloud } from "lucide-react";

import { Button } from "@/components/ui/button";

type ImagePanelProps = {
  selectedFile: File | null;
  imagePreview: string | null;
  imageResult: string | null;
  uploading: boolean;
  onFileChange: (file: File | null) => void;
  onUpload: (event: FormEvent<HTMLFormElement>) => void;
};

export function ImagePanel({
  selectedFile,
  imagePreview,
  imageResult,
  uploading,
  onFileChange,
  onUpload,
}: ImagePanelProps) {
  return (
    <form className="flex min-h-[34rem] flex-col gap-5 p-4" onSubmit={onUpload}>
      <label className="flex min-h-48 cursor-pointer flex-col items-center justify-center gap-3 rounded-lg border border-dashed bg-background/80 p-6 text-center transition-colors hover:bg-muted/50">
        <UploadCloud className="size-8 text-muted-foreground" />
        <span className="text-sm font-medium">
          {selectedFile ? selectedFile.name : "Choose image"}
        </span>
        <input
          className="sr-only"
          type="file"
          accept="image/*"
          onChange={(event) => onFileChange(event.target.files?.[0] ?? null)}
        />
      </label>

      {imagePreview && (
        <div className="overflow-hidden rounded-lg border bg-background">
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img
            src={imagePreview}
            alt={selectedFile?.name || "Selected image"}
            className="max-h-80 w-full object-contain"
          />
        </div>
      )}

      {imageResult && (
        <div className="rounded-lg border bg-background px-4 py-3">
          <p className="text-xs uppercase text-muted-foreground">Recognition</p>
          <p className="mt-1 text-lg font-semibold">{imageResult}</p>
        </div>
      )}

      <Button type="submit" className="gap-2" disabled={!selectedFile || uploading}>
        {uploading ? (
          <Loader2 className="size-4 animate-spin" />
        ) : (
          <ImageIcon className="size-4" />
        )}
        Recognize
      </Button>
    </form>
  );
}
